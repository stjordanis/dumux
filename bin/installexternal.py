#!/usr/bin/env python3

"""
install external stuff for dumux
"""
import os
import urllib.request
import tarfile
import sys
import subprocess
import shutil
import re
import argparse

class ChoicesAction(argparse._StoreAction):
    def __init__(self, **kwargs):
        super(ChoicesAction, self).__init__(**kwargs)
        if self.choices is None:
            self.choices = []
        self._choices_actions = []
    def add_choice(self, choice, help=''):
        self.choices.append(choice)
        # self.container.add_argument(choice, help=help, action='none')
        choice_action = argparse.Action(option_strings=[], dest=choice, help=help)
        self._choices_actions.append(choice_action)
    def _get_subactions(self):
        return self._choices_actions

parser = argparse.ArgumentParser(prog='installexternal',
                                 usage='./installexternal.py [OPTIONS] PACKAGES"',
                                 description='This script downloads dumux and dune extenstions\
                                     and installs some of External Libraries and Modules.')
parser.register('action', 'store_choice', ChoicesAction)
# Positional arguments
group = parser.add_argument_group(title='your choice of packages')
packages = group.add_argument('packages', nargs='*', metavar='PACKAGES',
                 action='store_choice')
packages.add_choice('all', help='Install everything and the kitchen sink.')
packages.add_choice('dumux-extensions', help="Download dumux-course and dumux-lecture.")
packages.add_choice('dune-extensions', help="Download dune-uggrid, dune-alugrid, dune-foamgrid, \
                    dune-subgrid, dune-spgrid, dune-mmesh and dune-functions.")
packages.add_choice('optimization', help="Download and install glpk and nlopt.")
packages.add_choice('others', help="Download and install opm , metis and gstat.")
packages.add_choice('lecture', help="Download dumux-lecture.")
packages.add_choice('course', help="Download dumux-course.")
packages.add_choice('ug', help="Download dune-uggrid.")
packages.add_choice('alugrid', help="Download dune-alugrid.")
packages.add_choice('foamgrid', help="Download dune-foamgrid.")
packages.add_choice('subgrid', help="Download dune-subgrid.")
packages.add_choice('spgrid', help="Download dune-spgrid.")
packages.add_choice('mmesh', help="Download dune-mmesh.")
packages.add_choice('functions', help="Download dune-functions.")
packages.add_choice('glpk', help="Download and install glpk.")
packages.add_choice('nlopt', help="Download and install nlopt.")
packages.add_choice('opm', help="Download opm modules required for cornerpoint grids.")
packages.add_choice('metis', help="Download and install the METIS graph partitioner.")
packages.add_choice('gstat', help="Download and install gstat.")


# Optional arguments
options = parser.add_mutually_exclusive_group(required=False)
options.add_argument('--clean', action="store_true", default=False,
                     help='Delete all files for the given packages.')
options.add_argument('--download', action="store_true", default=True,
                     help='Only download the packages.')
options.add_argument('--config', action="store_true", default=False,
                     help='Only config the packages.')
options.add_argument('--dune_branch', action="store_true", default="releases/2.7",
                     help='Dune branch to be checked out.')
options.add_argument('--dumux_branch', action="store_true", default="releases/3.2",
                     help='Dumux branch to be checked out.')

args = vars(parser.parse_args())

def show_message(message):
    print("*" * 120)
    print("")
    print(*message, sep='\n')
    print("")
    print("*" * 120)

def run_command(command):
    with open("../installexternal.log", "a") as log:
        popen = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        for line in popen.stdout:
            log.write(line)
            print(line, end='')
        for line in popen.stderr:
            log.write(line)
            print(line, end='')
        popen.stdout.close()
        popen.stderr.close()
        return_code = popen.wait()
        if return_code:
            print("\n")
            message = "\n    (Error) The command {} returned with non-zero exit code\n".format(command)
            message += "\n    If you can't fix the problem yourself consider reporting your issue\n"
            message += "    on the mailing list (dumux@listserv.uni-stuttgart.de) and attach the file 'installdumux.log'\n"
            show_message(message)
            sys.exit(1)

def git_clone(url, branch=None):
    clone = ["git", "clone"]
    if branch:
        clone += ["-b", branch]
    result = run_command(command=[*clone, url])

def install_external(args):

    dune_branch = args['dune_branch']
    dumux_branch = args['dumux_branch']
    packages = args['packages']
    cleanup = args['clean']
    download = args['download']
    config= args['config']

    top_dir = os.getcwd()
    ext_dir =  top_dir + "/external"

    # Prepare a list of packages
    packages = []
    for pkg in args['packages']:
        if pkg == 'all':
            packages.extend(sum(list(packagenames.values()),[]))
        elif pkg in packagenames:
            packages.extend(packagenames[pkg])
        else:
            packages.extend([key for key in external_urls.keys() if pkg in key])
    args['packages'] = packages

    # check Location For DuneModules
    if not os.path.isdir("dune-common"):
        show_message(
            "You have to call " + sys.argv[0] + " for " + sys.argv[1] + " from\n"
            "the same directory in which dune-common is located.\n"
            "You cannot install it in this folder.")
        return


    for package in packages:

        # Set the directory: create ext_dir for external packages
        if not any([re.compile(p).match(package) for p in ['dumux','dune']]):
            os.makedirs(ext_dir, exist_ok=True)
            os.chdir(ext_dir)

        # Set the branch
        if 'dumux' in package:
            branch = dumux_branch
        elif 'dune' in package:
            branch = dune_branch
        elif 'mmesh' in package:
            branch = "release/1.1"

        # Run the requested command
        if cleanup:
            if os.path.exists(package):
                # Remove
                shutil.rmtree(package)
                print("{} is removed.".format(package))
            else:
                # Print No-Folder-Remove message
                print("The folder {} does not exist.".format(package))
            continue

        if download:
            # Check if tarball
            tarball = external_urls[package].endswith('tar.gz')

            if not os.path.exists(package):

                if tarball:
                    # Download the tarfile
                    print("{} is being downloaded".format(package))
                    filedata = urllib.request.urlopen(external_urls[package])
                    datatowrite = filedata.read()

                    with open(ext_dir + "/" + package +".tar.gz", 'wb') as f:
                        f.write(datatowrite)

                else:
                    # Clone from repo
                    git_clone(external_urls[package], branch)
            else:
                if not tarball:
                    # Checkout to the requested branch
                    os.chdir(top_dir + "/" + package)
                    subprocess.Popen(["git", "checkout", branch])
                    print("-- Skip cloning {} because the folder already exists.".format(package))
                    continue

        # Config
        if config:
            # Extract
            tf = tarfile.open(package+".tar.gz")
            tf.extractall()

            # Rename
            shutil.move(os.path.commonprefix(tf.getnames()), package)

            # Start the configuration
            os.chdir(ext_dir + "/"+package)
            if package == 'gstat':
                with open('configure', 'r+') as f:
                    content = f.read()
                    f.seek(0)
                    f.truncate()
                    f.write(content.replace('doc/tex/makefile', ''))

            # Configuration
            configcmd = "./configure" if package != 'metis' else ["make", "config"]
            run_command(configcmd) #subprocess.run(configcmd)
            run_command("make") #subprocess.run("make")

            if os.path.exists(package):
                print("Successfully installed {}.".format(package))

        # Change to top_dir
        os.chdir(top_dir)

    # Prepare the list of messages to be shown.
    final_message = [messages[key][0] for key in args.keys() if args[key] is True]
    for package in packages:
        if package in messages.keys():
            final_message.append('\n[---'+package+'---]')
            final_message.extend(messages[package])

    return final_message


# clear the log file
open('installexternal.log', 'w').close()

#################################################################
#################################################################
## (1/3) Define th necessary packages and their urls
#################################################################
#################################################################
dune_git_baseurl = "https://gitlab.dune-project.org/"
dumux_git_baseurl = "https://git.iws.uni-stuttgart.de/dumux-repositories/"
external_urls = {
    "dumux-lecture": dumux_git_baseurl + "dumux-lecture.git",
    "dumux-course": dumux_git_baseurl + "dumux-course.git",
    "dune-uggrid": dune_git_baseurl + "/staging/dune-uggrid",
    "dune-alugrid": dune_git_baseurl + "extensions/dune-alugrid.git",
    "dune-foamgrid": dune_git_baseurl + "extensions/dune-foamgrid.git",
    "dune-subgrid": "https://git.imp.fu-berlin.de/agnumpde/dune-subgrid.git",
    "dune-spgrid": dune_git_baseurl + "extensions/dune-spgrid.git",
    "dune-mmesh": dune_git_baseurl + "samuel.burbulla/dune-mmesh.git",
    "dune-functions": dune_git_baseurl + "staging/dune-functions.git",
    "glpk": "http://ftp.gnu.org/gnu/glpk/glpk-4.60.tar.gz",
    "nlopt": "http://ab-initio.mit.edu/nlopt/nlopt-2.4.2.tar.gz",
    "opm-common": "https://github.com/OPM/opm-common",
    "opm-grid": "https://github.com/OPM/opm-grid",
    "metis": "http://glaros.dtc.umn.edu/gkhome/fetch/sw/metis/metis-5.1.0.tar.gz",
    "gstat": "http://gstat.org/gstat.tar.gz",
}

packagenames = {
    "dumux-extensions": ["dumux-lecture", "dumux-course"],
    "dune-extensions": ["dune-uggrid", "dune-alugrid", "dune-foamgrid",
                        "dune-subgrid", "dune-spgrid", "dune-mmesh",
                        "dune-functions"],
    "optimization": ["glpk", "nlopt"],
    "others": ["opm-common", "opm-grid", "metis", "gstat"]
}

messages ={
    'glpk': ["In addition, it might be necessary to set manually",
            "the glpk path in the CMAKE_FLAGS section of the .opts-file:",
            "  -DGLPK_ROOT=/path/to/glpk \\"],
    'opm-common': ["In addition, it might be necessary to set manually some",
                "CMake variables in the CMAKE_FLAGS section of the .opts-file:",
                "  -DUSE_MPI=ON", "",
                "Maybe you also have to install the following packages (see the",
                " opm prerequisites at opm-project.org):",
                "  BLAS, LAPACK, Boost, SuiteSparse, Zoltan"],
    'clean': ['The requested cleaning task has been completed.'],
    'download': ['The requested download task has been completed.'],
    'config': ['The requested installation task has been completed.']
}


#################################################################
#################################################################
## (2/3) Download/Config/Clean the requested packages
#################################################################
#################################################################
# Start download/configuration/cleaning tasks
final_message = install_external(args)

#################################################################
#################################################################
## (3/3) Show the final message
#################################################################
#################################################################
# Show final message
show_message(final_message)
