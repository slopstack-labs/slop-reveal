// Windows-only POSIX getopt_long shim.
// Included via conditional in config.cpp; never compiled on POSIX systems.
#pragma once
#ifdef _WIN32

#include <cstring>
#include <cstdio>

#ifndef no_argument
#  define no_argument       0
#  define required_argument 1
#  define optional_argument 2
#endif

struct option {
    const char* name;
    int         has_arg;
    int*        flag;
    int         val;
};

static int   optind = 1;
static int   opterr = 1;
static int   optopt = 0;
static char* optarg = nullptr;

// Intra-cluster scan position for short-option bundles like -abc.
// Reset whenever the caller resets optind.
static int _getopt_sp = 1;

static int getopt_long(int argc, char* const argv[],
                        const char* optstring,
                        const struct option* longopts,
                        int* longindex)
{
    static int _prev_optind = 1;
    if (optind <= 1 || optind < _prev_optind) _getopt_sp = 1;
    _prev_optind = optind;

    for (;;) {
        if (optind >= argc) return -1;

        if (_getopt_sp == 1) {
            const char* tok = argv[optind];
            if (!tok || tok[0] != '-' || tok[1] == '\0') return -1;

            if (tok[1] == '-') {
                if (tok[2] == '\0') { ++optind; return -1; }   // bare "--"

                // Long option
                const char* name = tok + 2;
                const char* eq   = std::strchr(name, '=');
                std::size_t nlen = eq ? (std::size_t)(eq - name) : std::strlen(name);

                for (int i = 0; longopts && longopts[i].name; ++i) {
                    if (std::strlen(longopts[i].name) == nlen &&
                        std::strncmp(longopts[i].name, name, nlen) == 0)
                    {
                        if (longindex) *longindex = i;
                        ++optind;
                        optarg = nullptr;
                        if (longopts[i].has_arg == required_argument) {
                            if (eq) {
                                optarg = const_cast<char*>(eq + 1);
                            } else if (optind < argc) {
                                optarg = argv[optind++];
                            } else {
                                if (opterr)
                                    std::fprintf(stderr,
                                        "option '--%.*s' requires an argument\n",
                                        (int)nlen, name);
                                optopt = longopts[i].val;
                                return '?';
                            }
                        } else if (longopts[i].has_arg == optional_argument && eq) {
                            optarg = const_cast<char*>(eq + 1);
                        }
                        if (longopts[i].flag) {
                            *longopts[i].flag = longopts[i].val;
                            return 0;
                        }
                        return longopts[i].val;
                    }
                }
                if (opterr)
                    std::fprintf(stderr, "unrecognized option '--%.*s'\n", (int)nlen, name);
                ++optind;
                return '?';
            }
        }

        // Short option
        const char* tok = argv[optind];
        char c = tok[_getopt_sp];
        if (!c) { ++optind; _getopt_sp = 1; continue; }

        const char* p = std::strchr(optstring, c);
        if (!p || c == ':') {
            optopt = static_cast<unsigned char>(c);
            if (tok[++_getopt_sp] == '\0') { ++optind; _getopt_sp = 1; }
            if (opterr) std::fprintf(stderr, "invalid option -- '%c'\n", c);
            return '?';
        }

        if (p[1] == ':') {
            optarg = nullptr;
            if (tok[_getopt_sp + 1]) {
                optarg = const_cast<char*>(tok + _getopt_sp + 1);
            } else if (++optind < argc) {
                optarg = argv[optind];
            } else {
                optopt = static_cast<unsigned char>(c);
                if (opterr) std::fprintf(stderr, "option '-%c' requires an argument\n", c);
                ++optind; _getopt_sp = 1;
                return (optstring[0] == ':') ? ':' : '?';
            }
            ++optind; _getopt_sp = 1;
        } else {
            optarg = nullptr;
            if (tok[++_getopt_sp] == '\0') { ++optind; _getopt_sp = 1; }
        }

        return static_cast<unsigned char>(c);
    }
}

#endif  // _WIN32
