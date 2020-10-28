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

class install:

    def __init__(self):
        self.CORRECT_LOCATION_FOR_DUNE_MODULES = False
        self.ENABLE_MPI = False
        self.ENABLE_DEBUG = False
        self.ENABLE_PARALLEL = False
        self.CLEANUP = False
        self.DOWNLOAD_ONLY = False
        self.DUNE_VERSION = "2.7"
        self.DUMUX_VERSION = "3.2"

        self.TOPDIR = os.getcwd()
        self.EXTDIR = os.getcwd() + "/external"

    def __help__(self):
        print("")
        print("help: ./installexternal.py [OPTIONS] PACKAGES")
        print("")
        print("where PACKAGES is one or more of the following")
        print("multiple packages can be downloaded as a group:")
        print("  all               Install everything and the kitchen sink.")
        print("  dumux-extensions  Download dumux-course and dumux-lecture.")
        print("  dune-extensions   Download dune-uggrid, dune-alugrid, dune-foamgrid,")
        print("                    dune-subgrid, dune-spgrid, dune-mmesh and dune-functions.")
        print("  optimization      Download and install glpk and nlopt.")
        print("  others            Download and install opm , metis and gstat.")
        print("")
        print("... or individually as:")
        print("  lecture           Download dumux-lecture.")
        print("  course            Download dumux-course.")

        print("")
        print("  ug                Download dune-uggrid.")
        print("  alugrid           Download dune-alugrid.")
        print("  foamgrid          Download dune-foamgrid.")
        print("  subgrid           Download dune-subgrid.")
        print("  spgrid            Download dune-spgrid.")
        print("  mmesh             Download dune-mmesh.")
        print("  functions         Download dune-functions.")

        print("")
        print("  glpk             Download and install glpk.")
        print("  nlopt            Download and install nlopt.")

        print("")
        print("  opm              Download opm modules required for cornerpoint grids.")
        print("  metis            Install the METIS graph partitioner.")
        print("  gstat            Download and install gstat.")

        print("")
        print("The following options are recoginzed:")
        print("    --parallel       Enable parallelization if available.")
        print("    --debug          Compile with debugging symbols and without optimization.")
        print("    --clean          Delete all files for the given packages.")
        print("    --download       Only download the packages.")

    def __checkLocationForDuneModules__(self):
        if not os.path.isdir("dune-common"):
            print("You have to call " + sys.argv[0] + " for " + sys.argv[1] + " from")
            print("the same directory in which dune-common is locatied.")
            print("You cannot install it in this folder")
            return (False)
        else:
            return(True)

    def __createExternalDirectory__(self):
        if not (os.path.exists(self.EXTDIR)):
            os.mkdir(self.EXTDIR)
        else:
            return
    def __git__(self,*args):
        return subprocess.check_call(['git'] + list(args))

    def installAluGrid(self):
        os.chdir(self.TOPDIR)
        self.__checkLocationForDuneModules__()
        print("Function installAluGrid is used")
        print("correct location for dune-modules is " )
        print(self.CORRECT_LOCATION_FOR_DUNE_MODULES)
        if not self.__checkLocationForDuneModules__():
            return

        if not os.path.exists("dune-alugrid"):
            print("dune-alugrid is cloned")
            self.__git__("clone", "https://gitlab.dune-project.org/extensions/dune-alugrid.git", "-b", "releases/2.4")

        if self.DOWNLOAD_ONLY:
            return

        if (self.CLEANUP and os.path.exists("dune-alugrid")):
            shutil.rmtree('dune-alugrid')
            print("alugrid is removed")

    def installCourse(self):
        os.chdir(self.TOPDIR)
        if not self.__checkLocationForDuneModules__():
            return

        if not os.path.exists("dumux-course"):
            print("dumux-course is cloned")
            self.__git__("clone", "https://git.iws.uni-stuttgart.de/dumux-repositories/dumux-course.git", "-b", "releases/" + self.DUMUX_VERSION)
        else:
            os.chdir(self.TOPDIR + "/dumux-course")
            subprocess.Popen(["git", "checkout", "releases/" + self.DUMUX_VERSION])
            print ("Skip cloning dumux-course because the folder already exists.")

    def installFoamGrid(self):
        os.chdir(self.TOPDIR)
        if not self.__checkLocationForDuneModules__():
            return
        if not os.path.exists("dune-foamgrid"):
            print("dune-foam is cloned")
            self.__git__("clone", "https://gitlab.dune-project.org/extensions/dune-foamgrid.git", "-b", "releases/2.4")

        if self.DOWNLOAD_ONLY:
            return

        if (self.CLEANUP and os.path.exists("dune-foamgrid")):
            shutil.rmtree('dune-foamgrid')
            print("foamgrid is removed")

    def installGLPK(self):
        self.__createExternalDirectory__()
        os.chdir(self.EXTDIR)

        if not os.path.exists("glpk-4.60.tar.gz"):
            print("glpk is installed")
            filedata = urllib.request.urlopen('http://ftp.gnu.org/gnu/glpk/glpk-4.60.tar.gz')
            datatowrite = filedata.read()

            with open(self.EXTDIR + "/glpk-4.60.tar.gz", 'wb') as f:
                f.write(datatowrite)

        if self.DOWNLOAD_ONLY:
            return

        if (self.CLEANUP and os.path.exists("glpk")):
            shutil.rmtree('glpk')
            print("glpk is removed")
            return

        if not os.path.exists("glpk"):
            os.mkdir( "glpk" );
            tf = tarfile.open("glpk-4.60.tar.gz")
            tf.extractall(self.EXTDIR)
            os.rename("glpk-4.60", "glpk")

        os.chdir(self.EXTDIR + "/glpk")

        subprocess.call("./configure")
        subprocess.call("make")

        # show additional information
        print("In addition, it might be necessary to set manually" )
        print("the glpk path in the CMAKE_FLAGS section of the .opts-file:")
        print("  -DGLPK_ROOT=/path/to/glpk \\")

        os.chdir(self.TOPDIR)

    def installGStat(self):
        self.__createExternalDirectory__()

        os.chdir(self.EXTDIR)

        if not os.path.exists("gstat.tar.gz"):
            print( "gstat is installed")
            filedata = urllib.request.urlopen('http://gstat.org/gstat.tar.gz')
            datatowrite = filedata.read()

            with open(self.EXTDIR + "/gstat.tar.gz", 'wb') as f:
                f.write(datatowrite)

        if self.DOWNLOAD_ONLY:
            return

        if (self.CLEANUP and os.path.exists("gstat")):
            shutil.rmtree('gstat')
            print("gstat is removed")
            return

        tf = tarfile.open("gstat.tar.gz")

        tf.extractall()
        os.rename("standalone", "gstat")

        os.chdir(self.EXTDIR + "/gstat")
        with open('configure', 'r+') as f:
            content = f.read()
            f.seek(0)
            f.truncate()
            f.write(content.replace('doc/tex/makefile', ''))

        subprocess.call("./configure")
        subprocess.call("make")
        if os.path.exists("gstat"):
            print("Successfully installed gstat")

    def installLecture(self):
        os.chdir(self.TOPDIR)
        if not self.__checkLocationForDuneModules__():
            return
        if not os.path.exists("dumux-lecture"):
            print("dumux-lecture is cloned")
            self.__git__("clone", "https://git.iws.uni-stuttgart.de/dumux-repositories/dumux-lecture.git", "-b", "releases/" + self.DUMUX_VERSION)
        else:
            os.chdir(self.TOPDIR + "/dumux-lecture")
            subprocess.Popen(["git", "checkout", "releases/" + self.DUMUX_VERSION])
            print ("Skip cloning dumux-lecture because the folder already exists.")

    def installMETIS(self):
        self.__createExternalDirectory__()
        os.chdir(self.EXTDIR)

        if not os.path.exists("metis-5.1.0.tar.gz"):
            print("metis-5.1.0 is cloned")
            os.mkdir( "metis-5.1.0" )
            filedata = urllib.request.urlopen('http://glaros.dtc.umn.edu/gkhome/fetch/sw/metis/metis-5.1.0.tar.gz')
            datatowrite = filedata.read()

            with open(self.EXTDIR + "/metis-5.1.0.tar.gz", 'wb') as f:
                f.write(datatowrite)

        if self.DOWNLOAD_ONLY:
            return

        if (self.CLEANUP and os.path.exists("metis-5.1.0")):
            shutil.rmtree('metis-5.1.0')
            print("metis-5.1.0 is removed")
            return


        tf = tarfile.open("metis-5.1.0.tar.gz")
        tf.extractall(self.EXTDIR)

        os.chdir(self.EXTDIR + "/metis-5.1.0")

        subprocess.run(["make", "config"])
        subprocess.call("make")
        os.chdir(self.EXTDIR)

    def installNLOPT(self):
        self.__createExternalDirectory__()
        os.chdir(self.EXTDIR)

        if not os.path.exists("nlopt-2.4.2.tar.gz"):
            print("nlopt is cloned")
            filedata = urllib.request.urlopen('http://ab-initio.mit.edu/nlopt/nlopt-2.4.2.tar.gz')
            datatowrite = filedata.read()

            with open(self.EXTDIR + "/nlopt-2.4.2.tar.gz", 'wb') as f:
                f.write(datatowrite)

        if self.DOWNLOAD_ONLY:
            return

        if (self.CLEANUP and os.path.exists("nlopt")):
            shutil.rmtree('nlopt')
            print("nlopt is removed")
            return

        if not os.path.exists("nlopt"):
            os.mkdir( "nlopt" );
            tf = tarfile.open("nlopt-2.4.2.tar.gz")
            tf.extractall(self.EXTDIR)
            os.rename("nlopt-2.4.2", "nlopt")

        os.chdir(self.EXTDIR + "/nlopt")

        subprocess.call("./configure")
        print("configure done")
        subprocess.call("make")
        print("make done")
        if os.path.exists("nlopt"):
            print("Successfully isntalled nlopt")
        os.chdir(self.TOPDIR)

    def installOPM(self):
        self.__createExternalDirectory__()
        os.chdir(self.TOPDIR)

        if not self.__checkLocationForDuneModules__():
            return

        if not os.path.exists("opm-common"):
            print("opm-common is cloned")
            self.__git__("clone", "https://github.com/OPM/opm-common", "-b", "release/2020.04")
            os.chdir(self.TOPDIR)
        if not os.path.exists("opm-grid"):
            print("opm-grid is cloned")
            self.__git__("clone", "https://github.com/OPM/opm-grid", "-b", "release/2020.04")
            os.chdir(self.TOPDIR)

        if self.DOWNLOAD_ONLY:
            return

        if (self.CLEANUP and os.path.exists("dune-multidomaingrid")):
            shutil.rmtree('opm-common')
            shutil.rmtree('opm-grid')
            print("opm-common and opm-grid are removed")

        # show additional information
        print("In addition, it might be necessary to set manually some")
        print("CMake variables in the CMAKE_FLAGS section of the .opts-file:")
        print("  -DUSE_MPI=ON")

        # show some opm prerequisites
        print("Maybe you also have to install the following packages (see the opm prerequisites at opm-project.org): ")
        print("  BLAS, LAPACK, Boost, SuiteSparse, Zoltan")
        os.chdir(self.TOPDIR)

    def installUG(self):
        if not self.__checkLocationForDuneModules__():
            return

        os.makedirs(self.EXTDIR, exist_ok=True)
        os.chdir(self.EXTDIR)
        UG_VERSION="3.12.1"

        if not os.path.exists("ug-" + UG_VERSION):
            print("dune-uggrid-{} is cloned".format(UG_VERSION))
            self.__git__("clone", "-b", "v" + UG_VERSION, "https://gitlab.dune-project.org/staging/dune-uggrid.git", "ug-"+UG_VERSION)

        if self.DOWNLOAD_ONLY:
            return

        if (self.CLEANUP and os.path.exists("ug-" + UG_VERSION)):
            shutil.rmtree("ug-" + UG_VERSION)
            print("dune-uggrid is removed")
            return

    def installSubGrid(self):
        os.chdir(self.TOPDIR)
        if not self.__checkLocationForDuneModules__():
            return
        if not os.path.exists("dune-subgrid"):
            print("dune-subgrid is cloned")
            self.__git__("clone", "https://git.imp.fu-berlin.de/agnumpde/dune-subgrid.git", "-b", "releases/" + self.DUNE_VERSION)

        if self.DOWNLOAD_ONLY:
            return

        if (self.CLEANUP and os.path.exists("dune-subgrid")):
            shutil.rmtree('dune-subgrid')
            print("subgrid is removed")


    def installSpGrid(self):
        os.chdir(self.TOPDIR)
        if not self.__checkLocationForDuneModules__():
            return

        if not os.path.exists("dune-spgrid"):
            print("dune-spgrid is cloned")
            self.__git__("clone", "https://gitlab.dune-project.org/extensions/dune-spgrid.git", "-b", "releases/" + self.DUNE_VERSION)

        if self.DOWNLOAD_ONLY:
            return

        if (self.CLEANUP and os.path.exists("dune-spgrid")):
            shutil.rmtree('dune-spgrid')
            print("spgrid is removed")


    def installMmesh(self):
        os.chdir(self.TOPDIR)
        if not self.__checkLocationForDuneModules__():
            return

        if not self.os.path.exists("dune-mmesh"):
            print("dune-mmesh is cloned")
            self.__git__("clone", "https://gitlab.dune-project.org/samuel.burbulla/dune-mmesh.git", "-b", "release/1.1")

        if self.DOWNLOAD_ONLY:
            return

        if (self.CLEANUP and os.path.exists("dune-mmesh")):
            shutil.rmtree('dune-mmesh')
            print("mmesh is removed")

    def installFunctions(self):
        os.chdir(self.TOPDIR)
        if not self.__checkLocationForDuneModules__():
            return

        if not os.path.exists("dune-functions"):
            print("dune-functions is cloned")
            self.__git__("clone", "https://gitlab.dune-project.org/staging/dune-functions.git", "-b", "releases/" + self.DUNE_VERSION)

        if self.DOWNLOAD_ONLY:
            return

        if (self.CLEANUP and os.path.exists("dune-functions")):
            shutil.rmtree('dune-functions')
            print("dune-functions is removed")

    def installDumuxExtensions(self):
        print("dumux-extensions are downloaded, this includes: dumux-course and dumux-lecture")
        self.installLecture()
        self.installCourse()

    def installDuneExtensions(self):
        print("dune-extensions are downloaded, this includes: dune-uggrid, dune-alugrid, "
              "dune-foamgrid, dune-subgrid, dune-spgrid, dune-mmesh and dune-functions")
        self.installUG()
        self.installAluGrid()
        self.installFoamGrid()
        self.installSubGrid()
        self.installSpGrid()
        self.installMmesh()
        self.installFunctions()

    def installOptimizationStuff(self):
        print("libraries for solving optimization problems are installed, this includes:"
              "glpk and nlopt")
        self.__createExternalDirectory__()
        self.installGLPK()
        self.installNLOPT()

    def installOthers(self):
        print("other libraries are installed, this includes: opm, metis and gstat")
        self.__createExternalDirectory__()
        self.installOPM()
        self.installMETIS()
        self.installGStat()

def call_method(obj, arguments):

    # Set the options
    if "--debug" in arguments:
        obj.ENABLE_DEBUG = True
    if "--clean" in arguments:
        obj.CLEANUP = True
    if "--download" in arguments:
        obj.DOWNLOAD_ONLY = True
    if "--parallel" in arguments:
        obj.ENABLE_PARALLEL = True
        obj.ENABLE_MPI = True
        obj.MPICC = subprocess.check_output(["which", "mpicc"]).decode().strip()
        obj.MPICXX = subprocess.check_output(["which", "mpicxx"]).decode().strip()
        obj.MPIF77 = subprocess.check_output(["which", "mpif77"]).decode().strip()

    # Get the list of methods
    method_list = [func for func in dir(obj) if callable(getattr(obj, func))]

    # Show help
    if len(arguments)==1 or arguments[1] == "-h":
        obj.__help__()
        return

    # Select function names
    if "all" in arguments:
        print("All external modules are installed")
        functions = ['installDumuxExtensions', 'installDuneExtensions',
                     'installOptimizationStuff', 'installOthers']
    else:
        functions = []
        for arg in arguments:
            functions.extend([s for s in method_list if "install"+arg.replace('-', '') in s.lower()])

    # Call the functions
    for function in functions:
        getattr(obj, function)()

    return

if __name__ == "__main__":

    # Call the class
    installexternal = install()

    # Choose your desired version
    installexternal.DUNE_VERSION = "2.7"
    installexternal.DUMUX_VERSION = "3.2"

    # Downloads and installations
    call_method(installexternal, sys.argv)
