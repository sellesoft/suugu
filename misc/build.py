#!/usr/bin/python
#_____________________________________________________________________________________________________
#                                            Usage
#_____________________________________________________________________________________________________
# build.py <command> [arguments...]
#
# Commands:
#   (empty)       Compile and link (default)
#   compile       Compile only
#   link          Link with previously generated .obj/.o
#   one <file>    Compile one file and link with previously generated .obj/.o
#
# Arguments:
#   --v    Echo build commands to the console
#   --time Time the script (this relies on GNU awk)
#   --d    Build with    debug info and without optimization (default)
#   --r    Build without debug info and with    optimization
#   --p    Enable Tracy profiling
#   --pw   Enable Tracy profiling and force the program to wait for a connection to start running
#   --sa   Enable static analysis
#
#   -platform <win32,mac,linux>           Build for specified OS: win32, mac, linux (default: builder's OS)
#   -graphics <vulkan,opengl,directx>     Build for specified Graphics API (default: vulkan)
#   -compiler <cl,gcc,clang,clang-cl>     Build using the specified compiler (default: cl on Windows, gcc on Mac and Linux)
#   -linker <link,ld,lld,lld-link>        Build using the specified linker (default: link on Windows, ld on Mac and Linux)
#   -vulkan_path <path_to_vulkan>         Override the default $VULKAN_SDK path with this path

import os,sys,subprocess,platform,time
from datetime import datetime
from threading import Thread

app_name = "suugu"

config = {
    "verbose": False, 
    "time": False,
    "buildmode": "debug",
    "profiling": "off", # "off", "on", "on and wait"
    "static_analysis": False,

    "platform": "unknown",
    "compiler": "unknown",
    "linker":   "unknown",
    "graphics": "vulkan",
}

match platform.system():
    case 'Windows': 
        config["platform"] = "win32"
        config["compiler"] = "cl"
        config["linker"] = "link"
    case 'Linux': 
        config["platform"] = "linux"
        config["compiler"] = "clang++"
        config["linker"] = "ld"
    case _:
        # TODO(sushi) if we ever setup cross compiling somehow, we need to remove this
        print(f"unsupported platform: {platform.system()}")

#
#  gather command line arguments
#______________________________________________________________________________________________________________________

i = 0
while i < len(sys.argv):
    match sys.argv[i]:
        case "--v":    config["verbose"] = True
        case "--time": config["time"] = True
        case "--d":    continue # dont do anything cause debug is the default
        case "--r":    config["buildmode"] = "release"
        case "--p":    config["profiling"] = "on"
        case "--pw":   config["profiling"] = "on and wait"
        case "--sa":   config["static_analysis"] = True

        case "-platform":
            if i != len(sys.argv) - 1:
                i += 1
                config["platform"] = sys.argv[i]
                if config["platform"] not in ("win32", "linux", "mac"):
                    print(f"unknown platform: {sys.argv[i]}, expected one of (win32, linux, mac).")
                    quit()
            else:
                print("expected a platform (win32, linux, max) after switch '-platform'")

        case "-graphics":
            if i != len(sys.argv[i]):
                i += 1
                config["graphics"] = sys.argv[i]
                if config["graphics"] not in ("vulkan", "opengl", "directx"):
                    print(f"unknown api backend: {sys.argv[i]}, expected one of (vulkan, opengl, directx).")
                    quit()
            else:
                print("expected a graphics api (vulkan, opengl, directx) after switch '-graphics'")

        case "-compiler":
            if i != len(sys.argv[i]):
                i += 1
                config["compiler"] = sys.argv[i]
                if config["compiler"] not in ("cl", "gcc", "clang", "clang-cl"):
                    print(f"unknown compiler: {sys.argv[i]}, expected one of (cl, gcc, clang, clang-cl).")
                    quit()
            else:
                print("expected a compiler (cl, gcc, clang, clang-cl) after switch '-compiler'")

        case "-linker":
            if i != len(sys.argv[i]):
                i += 1
                config["linker"] = sys.argv[i]
                if config["linker"] not in ("link", "ld", "lld", "lld-link"):
                    print(f"unknown linker: {sys.argv[i]}, expected one of (link, ld, lld, lld-link).")
                    quit()
            else:
                print("expected a linker (cl, gcc, clang, clang-cl) after switch '-linker'")
        case _:
            if sys.argv[i].startswith("-"):
                print(f"unknown switch: {sys.argv[i]}")
                quit()
    i += 1
# end of cli arg collection

# determines what compilers and linkers are compatible with each platform
# so that extending this is easy
compatibility = {
    "win32": {
        "compiler": ["cl", "clang-cl"],
        "linker": ["link"]
    },
    "linux":{
        "compiler": ["clang", "gcc", "clang++"],
        "linker": ["ld", "lld", "lld-link", "clang++", "clang"]
    }
}

# setup folders
# this assumes that the build script is in a misc folder that is in the root of the repo
folders = {}
folders["misc"] = os.path.dirname(__file__)
folders["root"] = f"{folders['misc']}/.."
folders["build"] = f"{folders['root']}/build/{config['buildmode']}"

os.chdir(folders["root"])

#
#  data
#______________________________________________________________________________________________________________________

includes = (
    "-Isrc "
    "-Ideshi/src "
    "-Ideshi/src/external "
)

if config["graphics"] == "vulkan":
    includes += f''

sources = {
    "deshi": "deshi/src/deshi.cpp",
    "app": "src/suugu.cpp"
}

parts = {

    "gdbinit sources":[
        f"{folders['root']}/deshi/src/deshigdb.py",
        f"{folders['root']}/deshi/src/kigu/kigugdb.py",
        f"{folders['root']}/misc/debug/debug.py"
    ],

    "link":{
        "win32": {
            "always": ["gdi32", "shell32", "ws2_32", "winmm"],
            "vulkan": ["vulkan-1", "shaderc_combined"],
            "opengl": ["opengl32"],
            "paths": []
        },
        "linux": {
            "always": ["X11", "Xrandr"],
            "vulkan": ["vulkan", "shaderc_combined"],
            "opengl": ["GL"],
            "paths": []
        },
        "flags":{
            **dict.fromkeys(["link", "lld-link"], ( # NOTE(sushi) this is just assigning this value to both link and lld-link, so it doesnt appear twice
                "-nologo "         # prevents microsoft copyright banner
                "-opt:ref "        # doesn't link functions and data that are never used 
                "-incremental:no " # relink everything
            )),
            **dict.fromkeys(["ld", "lld"], "")
        },
        "prefix":{
            **dict.fromkeys(["link", "lld-link"], {
                "path": "-libpath:",
                "file": "",
            }),
            **dict.fromkeys(["ld", "lld"], {
                "path": "-L",
                "file": "-l",
            }),
        }
    },


    "defines":{
        "buildmode": {
            "release": "-DBUILD_INTERNAL=0 -DBUILD_SLOW=0 -DBUILD_RELEASE=1 ",
            "debug": "-DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_RELEASE=0 ",
        },
        "platform":{
            "win32": "-DDESHI_WINDOWS=1 -DDESHI_MAC=0 -DDESHI_LINUX=0 ",
            "linux": "-DDESHI_WINDOWS=0 -DDESHI_MAC=0 -DDESHI_LINUX=1 ",
            "mac":   "-DDESHI_WINDOWS=0 -DDESHI_MAC=1 -DDESHI_LINUX=0 ",
        },
        "graphics":{
            "vulkan":  "-DDESHI_VULKAN=1 -DDESHI_OPENGL=0 -DDESHI_DIRECTX12=0 ",
            "opengl":  "-DDESHI_VULKAN=0 -DDESHI_OPENGL=1 -DDESHI_DIRECTX12=0 ",
            "directx": "-DDESHI_VULKAN=0 -DDESHI_OPENGL=0 -DDESHI_DIRECTX12=1 ",
        },
        "profiling":{
            "on":          "-DTRACY_ENABLE ",
            "on and wait": "-DTRACY_ENABLE -DDESHI_WAIT_FOR_TRACY_CONNECTION ",
            "off": "",
        },
    },


    "compiler_flags":{

        "cl": {
            "always":( # flags always applied if this compiler is chosen
                "-diagnostics:column " # prints diagnostics on one line, giving column number
                "-EHsc "               # enables C++ exception handling and defaults 'extern "C"' to nothrow
                "-nologo "             # prevents initial banner from displaying
                "-MD "                 # create a multithreaded dll
                "-Oi "                 # generates intrinsic functions
                "-GR "                 # enables runtime type information
                "-std:c++17 "          # use C++17
                "-utf-8 "              # read source as utf8
                "-MP "                 # builds multiple source files concurrently
                "-W1"                  # warning level 1
                "-wd4100"              # unused function parameter
                "-wd4189"              # unused local variables
                "-wd4201"              # nameless unions and structs
                "-wd4311"              # pointer truncation
                "-wd4706"              # assignment within conditional expression
            ),
            "release": (
                "-O2 " # maximizes speed (O1 minimizes size)
            ),
            "debug": (
                "-Z7 " # generates C 7.0-compatible debugging information
                "-Od " # disables optimization complete
            )
        },

        "clang-cl": {
            "always":( # flags always applied if this compiler is chosen
                "-diagnostics:column "  # prints diagnostics on one line, giving column number
                "-EHsc "                # enables C++ exception handling and defaults 'extern "C"' to nothrow
                "-nologo "              # prevents initial banner from displaying
                "-MD "                  # create a multithreaded dll
                "-Oi "                  # generates intrinsic functions
                "-GR "                  # enables runtime type information
                "-std:c++17 "           # use C++17
                "-utf-8 "               # read source as utf8
                "-msse3 "               # enables SSE
                "-Wno-unused-value "                # 
                "-Wno-writable-strings "            # conversion from string literals to other things
                "-Wno-implicitly-unsigned-literal " # 
                "-Wno-nonportable-include-path "    # 
                "-Wno-unused-function "             #
                "-Wno-unused-variable "             #
                "-Wno-undefined-inline "            #
            ),
            "release": (
                "-O2 " # maximizes speed (O1 minimizes size)
            ),
            "debug": (
                "-Z7 " # generates C 7.0-compatible debugging information
                "-Od " # disables optimization completely
            ),
            "analyze": "--analyze "
        },

        "gcc": {
            "not implemented yet"
        },

        "clang++":{
            "always": ( # flags always applied if this compiler is chosen
                "-std=c++17 "         # use the c++17 standard
                "-fexceptions "       # enable exception handling
                "-fcxx-exceptions "   # enable c++ exceptions
                "-finline-functions " # inlines suitable functions
                "-pipe "              # use pipes between commands when possible
                "-msse3 "             # enables sse
                "-Wno-unused-value "  
                "-Wno-implicitly-unsigned-literal "
                "-Wno-nonportable-include-path "
                "-Wno-writable-strings "
                "-Wno-unused-function "
                "-Wno-unused-variable "
                "-Wno-undefined-inline "
                "-Wno-return-type-c-linkage "
            ),
            "release":(
                "-O2 " # maximizes speed (O1 minimizes size)
            ),
            "debug":(
                "-fdebug-macro " # output macro information
                "-ggdb3 " # output debug information for gdb
                "-O0 " # disable optimization completely
            )
        }
    }
}

# make sure that all the chosen options are compatible
if config["compiler"] not in compatibility[config["platform"]]["compiler"]:
    print(f"compiler {config['compiler']} is not compatible with the platform {config['platform']}")
    quit()
if config["linker"] not in compatibility[config["platform"]]["linker"]: 
    print(f"linker {config['linker']} is not compatible with the platform {config['platform']}")
    quit()

#
#  construct compiler commands
#______________________________________________________________________________________________________________________

header = f'{datetime.now().strftime("%a, %h %d %Y, %H:%M:%S")} ({config["compiler"]}/{config["buildmode"]}/{config["graphics"]}) [{app_name}]'
print(header)
print('-'*len(header))

# special vulkan case where we need to grab the sdk path
if config["graphics"] == "vulkan":
    vpath = os.getenv("VULKAN_SDK", None)
    if vpath == None:
        print("the chosen graphics API is Vulkan, but the environment variable VULKAN_SDK is not set")
    parts["link"][config["platform"]]["paths"].append(f'{vpath}lib')
    includes += f'-I{vpath}include '

link = {
    "flags": "",
    "libs": "",
    "paths": ""
}

defines = ""

if config["graphics"] == "vulkan":
    if folders["vulkan"] == None:
        print("vulkan was selected as the graphics api, but the environment variable VULKAN_SDK is not set")
    includes += f"-I{folders['vulkan']} "
    link["pathnames"].append(f"{folders['vulkan']}lib")

defines += (
    parts["defines"]["buildmode"][config["buildmode"]] +
    parts["defines"]["platform"][config["platform"]] +
    parts["defines"]["graphics"][config["graphics"]] +
    parts["defines"]["profiling"][config["profiling"]]
)

link_names = (
    parts["link"][config["platform"]]["always"] +
    parts["link"][config["platform"]][config["graphics"]]
)

nameprefix = parts['link']['prefix'][config['linker']]['file']
for ln in link_names:
    link["libs"] += f"{nameprefix}{ln} "

link_paths = parts["link"][config["platform"]]["paths"]

pathprefix = parts['link']['prefix'][config['linker']]['path']
for lp in link_paths:
    link["paths"] += f"{pathprefix}{lp} "

full_deshi = (
    f'{config["compiler"]} -c '
    f'{sources["deshi"]} '
    f'{defines} '
    f'{includes} '
    f'{parts["compiler_flags"][config["compiler"]]["always"]} '
    f'{parts["compiler_flags"][config["compiler"]][config["buildmode"]]} '
    f'-o {folders["build"]}/deshi.o'
)

full_app = (
    f'{config["compiler"]} -c '
    f'{sources["app"]} '
    f'{defines} '
    f'{includes} '
    f'{parts["compiler_flags"][config["compiler"]]["always"]} '
    f'{parts["compiler_flags"][config["compiler"]][config["buildmode"]]} '
    f'-o {folders["build"]}/{app_name}.o' 
)

full_link = (
    f'{config["compiler"]} ' # NOTE(sushi) this should not be the compiler, but it is for now cause that is how it is on linux for me so fix later please
    f'{folders["build"]}/deshi.o {folders["build"]}/{app_name}.o '
    f'{link["flags"]} '
    f'{link["libs"]} '
    f'{link["paths"]} '
    f'-o {folders["build"]}/{app_name}'
)

def format_time(time):
    one_second = 1
    one_minute = 60*one_second
    if not time: return "0 seconds"
    if time > one_minute:
        n_minutes = time//one_minute
        return f"{n_minutes} minute{'s' if n_minutes != 1 else ''} {format_time(time-n_minutes*one_minute)}"
    return f"{time} second{'s' if time != 1 else ''}"

def run_proc(name, cmd):
    if config["verbose"]: print(cmd)
    start = time.time()
    dproc = subprocess.Popen(cmd.split(' '))
    dproc.communicate()
    taken = format_time(round(time.time()-start, 3))
    if not dproc.returncode:
        print(f'  \033[32m{name}\033[0m - {taken}')
    else:
        print(f'  \033[31m{name} failed to build\033[0m - {taken}')

start = time.time()
dproc = Thread(target=run_proc, args=("deshi", full_deshi))
aproc = Thread(target=run_proc, args=(app_name, full_app))
dproc.start()
aproc.start()
dproc.join()
aproc.join()

lproc = Thread(target=run_proc, args=("exe", full_link))
lproc.start()
lproc.join()

# gdbinit = open(f"{folders['build']}/.gdbinit", "w")
# for s in parts["gdbinit sources"]:
#     gdbinit.write(f"source {os.path.abspath(s)}\n")
# gdbinit.close()

print(f"time: {format_time(round(time.time()-start, 3))}")