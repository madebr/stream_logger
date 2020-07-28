/**
 *  Log stdin/stdout/stderr of subprocess
 *  Copyright (C) 2020 Maarten DB
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/asio.hpp>

#include <boost/filesystem.hpp>

#include <boost/process.hpp>
#include <boost/process/extend.hpp>

#include <boost/program_options.hpp>

#include <unistd.h>

#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace bp = boost::process;
namespace po = boost::program_options;

enum logfiletype {
    LOG_ARGS = 0,
    LOG_INPUT = 1,
    LOG_OUTPUT = 2,
    LOG_ERROR = 3,
};


const char *logfiletype_to_string(logfiletype type) {
    switch (type) {
        case LOG_ARGS:
            return "args";
        case LOG_INPUT:
            return "stdin";
        case LOG_OUTPUT:
            return "stdout";
        case LOG_ERROR:
            return "stderr";
    }
    return "";
}

std::string get_logfilename(const std::string &prefix, unsigned nb, logfiletype type) {
    std::ostringstream oss;
    oss << prefix << "_" << logfiletype_to_string(type) << "_" << std::setfill('0') << std::setw(3) << nb;
    return oss.str();
}

struct process_handler {
    std::unique_ptr<bp::child> child;
    bp::async_pipe pin;
    bp::async_pipe pout;
    bp::async_pipe perr;
    std::vector<char> buffer_pout;
    std::vector<char> buffer_perr;

    boost::asio::posix::stream_descriptor in;
    std::vector<char> buffer_stdin;

    std::ofstream olog_in;
    std::ofstream olog_out;
    std::ofstream olog_err;

    void on_exit(int exit_code, const std::error_code& ec) {
        if (ec) {
        }

#if defined(BOOST_POSIX_API)
        if (WIFEXITED(exit_code)) {
            exit_code = WEXITSTATUS(exit_code);
        }
#endif
        boost::system::error_code cec;
        in.close(cec);
        pin.close();
        pout.close();
        perr.close();
    }

    process_handler(boost::asio::io_context &ios, const std::string &exe, const std::vector<std::string> &args, const std::string &name_prefix, unsigned lognumber)
    : pin(ios)
    , pout(ios)
    , perr(ios)
    , buffer_pout(4096)
    , buffer_perr(4096)
    , in(ios, ::dup(STDIN_FILENO))
    , buffer_stdin(4096)
    , olog_in(get_logfilename(name_prefix, lognumber, LOG_INPUT))
    , olog_out(get_logfilename(name_prefix, lognumber, LOG_OUTPUT))
    , olog_err(get_logfilename(name_prefix, lognumber, LOG_ERROR)) {

        boost::filesystem::path exep = exe;
        if (!boost::filesystem::exists(exe)) {
            exep = bp::search_path(exe);
        }
        child = std::make_unique<bp::child>(bp::exe=exep, bp::args=args, bp::std_in=pin, bp::std_out=pout, bp::std_err=perr, ios, bp::on_exit=std::bind(&process_handler::on_exit, this, std::placeholders::_1, std::placeholders::_2));

        in.async_read_some(boost::asio::buffer(buffer_stdin), std::bind(&process_handler::on_stdin_available, this, std::placeholders::_1, std::placeholders::_2));
        pout.async_read_some(boost::asio::buffer(buffer_pout), std::bind(&process_handler::on_process_out_available, this, std::placeholders::_1, std::placeholders::_2));
        perr.async_read_some(boost::asio::buffer(buffer_perr), std::bind(&process_handler::on_process_err_available, this, std::placeholders::_1, std::placeholders::_2));

    }

    void on_stdin_available(const boost::system::error_code &ec, std::size_t size) {
        if (ec) {
            in.close();
            pin.close();
        } else {
            olog_in.write(buffer_stdin.data(), size);
            olog_in.flush();
            boost::asio::write(pin, boost::asio::buffer(buffer_stdin, size));
            in.async_read_some(boost::asio::buffer(buffer_stdin), std::bind(&process_handler::on_stdin_available, this, std::placeholders::_1, std::placeholders::_2));
        }
    }

    void on_process_out_available(const boost::system::error_code &ec, std::size_t size) {
        if (ec) {
            pout.close();
            olog_out.close();
        } else {
            olog_out.write(buffer_pout.data(), size);
            olog_out.flush();
            std::cout.write(buffer_pout.data(), size);
            std::cout.flush();
            pout.async_read_some(boost::asio::buffer(buffer_pout), [&] (const boost::system::error_code &ec, std::size_t size) { this->on_process_out_available(ec, size);});
        }
    }

    void on_process_err_available(const boost::system::error_code &ec, std::size_t size) {
        if (ec) {
            perr.close();
            olog_err.close();
        } else {
            olog_err.write(buffer_perr.data(), size);
            olog_err.flush();
            std::cerr.write(buffer_perr.data(), size);
            std::cerr.flush();
            perr.async_read_some(boost::asio::buffer(buffer_perr), std::bind(&process_handler::on_process_err_available, this, std::placeholders::_1, std::placeholders::_2));
        }
    }
};

int main(int argc, const char *argv[]) {
    int start_cmd = 1;

    for (; start_cmd < argc; ++start_cmd) {
        const char *arg = argv[start_cmd];
        if (strcmp(arg, "--") == 0) {
            break;
        }
    }
    ++start_cmd;

    po::positional_options_description positional_opts;
    po::options_description desc("Run and log process.\n\nstream_logger [--name_prefix PREFIX] -- PROGRAM [ARG ...]\n\nGeneric options");
    desc.add_options()
        ("help", "Output this help message")
        ("name_prefix", po::value<std::string>()->default_value("log"), "Output name prefix")
    ;

    po::variables_map vm;

    try {
        po::store(po::command_line_parser(start_cmd-1, argv).options(desc).positional(positional_opts).run(), vm);
        po::notify(vm);
    } catch (po::error &e) {
        std::cerr << e.what() << "\n";
        std::cerr << desc << "\n";
        return 1;
    }

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

    if (start_cmd >= argc) {
        std::cerr << "No output command given\n";
        return 1;
    }

    std::string name_prefix = vm["name_prefix"].as<std::string>();

    unsigned log_i = 0;

    while (true) {
        std::string ofn = get_logfilename(name_prefix, log_i, LOG_ARGS);
        boost::filesystem::file_status status = boost::filesystem::status(ofn);
        if (!boost::filesystem::exists(status)) {
            break;
        }
        log_i += 1;
    }

    auto exe = argv[start_cmd];
    std::vector<std::string> args;

    std::string afn = get_logfilename(name_prefix, log_i, LOG_ARGS);
    std::ofstream of_args(afn);
    of_args << "'" << exe << "' ";
    for(int i=start_cmd+1; i < argc; ++i) {
        args.push_back(argv[i]);
        of_args << "'" << argv[i] << "' ";
    }
    of_args.close();

    boost::asio::io_context ios;

    process_handler handler(ios, exe, args, name_prefix, log_i);

    ios.run();

    handler.child->wait();

    return handler.child->exit_code();
}
