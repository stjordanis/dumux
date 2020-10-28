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

def checkLocationForDuneModules():
    if not os.path.isdir("dune-common"):
        print("You have to call " + sys.argv[0] + " for " + sys.argv[1] + " from")
        print("the same directory in which dune-common is locatied.")
        print("You cannot install it in this folder")
        return (False)
    else:
        return(True)

def createExternalDirectory():
    if not (os.path.exists(EXTDIR)):
        os.mkdir( EXTDIR );
    else:
        return

def installAluGrid():
    os.chdir(TOPDIR)
    checkLocationForDuneModules()
    print("Function installAluGrid is used")
    print("correct location for dune-modules is " )
    print (CORRECT_LOCATION_FOR_DUNE_MODULES)
    if not checkLocationForDuneModules():
        return

    if not os.path.exists("dune-alugrid"):
        print("alugrid is cloned")
        git("clone", "https://gitlab.dune-project.org/extensions/dune-alugrid.git", "-b", "releases/2.4")

    if  DOWNLOAD_ONLY:
        return

    if  (CLEANUP and os.path.exists("dune-alugrid")):
        shutil.rmtree('dune-alugrid')
        print("alugrid is removed")

def installCourse():
    os.chdir(TOPDIR)
    if not checkLocationForDuneModules():
        return

    if not os.path.exists("dumux-course"):
        print("dumux-course")
        git("clone", "https://git.iws.uni-stuttgart.de/dumux-repositories/dumux-course.git", "-b", "releases/" + DUMUX_VERSION)
    else:
        os.chdir(TOPDIR + "/dumux-course")
        subprocess.Popen(["git", "checkout", "releases/" + DUMUX_VERSION])
        print ("Skip cloning dumux-course because the folder already exists.")

def installFoamGrid():
    os.chdir(TOPDIR)
    if not checkLocationForDuneModules():
        return
    if not os.path.exists("dune-foamgrid"):
        print("foamgrid is cloned")
        git("clone", "https://gitlab.dune-project.org/extensions/dune-foamgrid.git", "-b", "releases/2.4")

    if  DOWNLOAD_ONLY:
        return

    if  (CLEANUP and os.path.exists("dune-foamgrid")):
        shutil.rmtree('dune-foamgrid')
        print("foamgrid is removed")

def installGLPK():
    os.chdir(EXTDIR)

    if not os.path.exists("glpk-4.60.tar.gz"):
        filedata = urllib.request.urlopen('http://ftp.gnu.org/gnu/glpk/glpk-4.60.tar.gz')
        datatowrite = filedata.read()

        with open(EXTDIR + "/glpk-4.60.tar.gz", 'wb') as f:
            f.write(datatowrite)

    if  DOWNLOAD_ONLY:
        return

    if  (CLEANUP and os.path.exists("glpk")):
        shutil.rmtree('glpk')
        print("glpk is removed")
        return
    if not os.path.exists("glpk"):
        os.mkdir( "glpk" );
        tf = tarfile.open("glpk-4.60.tar.gz")
        tf.extractall(EXTDIR)
        os.rename("glpk-4.60", "glpk")

    os.chdir(EXTDIR + "/glpk")

    subprocess.call("./configure")
    subprocess.call("make")


    # show additional information
    print( "In addition, it might be necessary to set manually" )
    print( "the glpk path in the CMAKE_FLAGS section of the .opts-file:")
    print( "  -DGLPK_ROOT=/path/to/glpk \\")

    os.chdir(TOPDIR)

def installGStat():
    os.chdir(EXTDIR)
    if os.path.exists("gstat"):
        shutil.rmtree('gstat')

    if not os.path.exists("gstat.tar.gz"):
        filedata = urllib.request.urlopen('http://gstat.org/gstat.tar.gz')
        datatowrite = filedata.read()

        with open(EXTDIR + "/gstat.tar.gz", 'wb') as f:
            f.write(datatowrite)

    if  DOWNLOAD_ONLY:
        return

    tf = tarfile.open("gstat.tar.gz")

    tf.extractall()
    os.rename("standalone", "gstat")

    os.chdir(EXTDIR + "/gstat")
    with open('configure', 'r+') as f:
        content = f.read()
        f.seek(0)
        f.truncate()
        f.write(content.replace('doc/tex/makefile', ''))

    subprocess.call("./configure")
    subprocess.call("make")
    if os.path.exists("gstat"):
        print("Successfully isntalled gstat")

def installLecture():
    os.chdir(TOPDIR)
    if not checkLocationForDuneModules():
        return
    if not os.path.exists("dumux-lecture"):
        print("dumux-lecture is cloned")
        git("clone", "https://git.iws.uni-stuttgart.de/dumux-repositories/dumux-lecture.git", "-b", "releases/" + DUMUX_VERSION)
    else:
        os.chdir(TOPDIR + "/dumux-lecture")
        subprocess.Popen(["git", "checkout", "releases/" + DUMUX_VERSION])
        print ("Skip cloning dumux-lecture because the folder already exists.")

def installMETIS():
    os.chdir(EXTDIR)

    if not os.path.exists("metis-5.1.0.tar.gz"):
        os.mkdir( "metis-5.1.0" );
        filedata = urllib.request.urlopen('http://glaros.dtc.umn.edu/gkhome/fetch/sw/metis/metis-5.1.0.tar.gz')
        datatowrite = filedata.read()

        with open(EXTDIR + "/metis-5.1.0.tar.gz", 'wb') as f:
            f.write(datatowrite)

    if  DOWNLOAD_ONLY:
        return
    if  (CLEANUP and os.path.exists("metis-5.1.0")):
        shutil.rmtree('metis-5.1.0')
        print("metis-5.1.0 is removed")
        return


    tf = tarfile.open("metis-5.1.0.tar.gz")
    tf.extractall(EXTDIR)

    os.chdir(EXTDIR + "/metis-5.1.0")

    subprocess.run(["make", "config"])
    subprocess.call("make")
    os.chdir(EXTDIR)

def installNLOPT():
    os.chdir(EXTDIR)

    if not os.path.exists("nlopt-2.4.2.tar.gz"):
        filedata = urllib.request.urlopen('http://ab-initio.mit.edu/nlopt/nlopt-2.4.2.tar.gz')
        datatowrite = filedata.read()

        with open(EXTDIR + "/nlopt-2.4.2.tar.gz", 'wb') as f:
            f.write(datatowrite)

    if  DOWNLOAD_ONLY:
        return
    if  (CLEANUP and os.path.exists("nlopt")):
        shutil.rmtree('nlopt')
        print("nlopt is removed")
        return

    if not os.path.exists("nlopt"):
        os.mkdir( "nlopt" );
        tf = tarfile.open("nlopt-2.4.2.tar.gz")
        tf.extractall(EXTDIR)
        os.rename("nlopt-2.4.2", "nlopt")

    os.chdir(EXTDIR + "/nlopt")

    subprocess.call("./configure")
    print("configure done")
    subprocess.call("make")
    print("make done")
    if os.path.exists("nlopt"):
        print("Successfully isntalled nlopt")
    os.chdir(TOPDIR)

def installOPM():
    os.chdir(TOPDIR)
    if not checkLocationForDuneModules():
        return
    if not os.path.exists("opm-common"):
        print("opm-common is cloned")
        git("clone", "https://github.com/OPM/opm-common", "-b", "release/2020.04")
        os.chdir(TOPDIR)
    if not os.path.exists("opm-grid"):
        print("opm-grid is cloned")
        git("clone", "https://github.com/OPM/opm-grid", "-b", "release/2020.04")
        os.chdir(TOPDIR)

    if  DOWNLOAD_ONLY:
        return

    if  (CLEANUP and os.path.exists("dune-multidomaingrid")):
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
    os.chdir(TOPDIR)

def installUG():
    if not checkLocationForDuneModules():
        return

    os.makedirs(EXTDIR, exist_ok=True)
    os.chdir(EXTDIR)
    UG_VERSION="3.12.1"

    if not os.path.exists("ug-" + UG_VERSION):
        print("ug-{} is cloned".format(UG_VERSION))
        git("clone", "-b", "v" + UG_VERSION, "https://gitlab.dune-project.org/staging/dune-uggrid.git", "ug-"+UG_VERSION)

    if DOWNLOAD_ONLY:
        return

    if CLEANUP and os.path.exists("ug-" + UG_VERSION):
        shutil.rmtree("ug-" + UG_VERSION)
        print("dune-typetree is removed")
        return

def installSubGrid():
    os.chdir(TOPDIR)
    if not checkLocationForDuneModules():
        return
    if not os.path.exists("dune-subgrid"):
        print("subgrid is cloned")
        git("clone", "https://git.imp.fu-berlin.de/agnumpde/dune-subgrid.git", "-b", "releases/" + DUNE_VERSION)

    if  DOWNLOAD_ONLY:
        return

    if  (CLEANUP and os.path.exists("dune-subgrid")):
        shutil.rmtree('dune-subgrid')
        print("subgrid is removed")


def installSpGrid():
    os.chdir(TOPDIR)
    if not checkLocationForDuneModules():
        return
    if not os.path.exists("dune-spgrid"):
        print("subgrid is cloned")
        git("clone", "https://gitlab.dune-project.org/extensions/dune-spgrid.git", "-b", "releases/" + DUNE_VERSION)

    if  DOWNLOAD_ONLY:
        return

    if  (CLEANUP and os.path.exists("dune-spgrid")):
        shutil.rmtree('dune-spgrid')
        print("spgrid is removed")


def installMmesh():
    os.chdir(TOPDIR)
    if not checkLocationForDuneModules():
        return
    if not os.path.exists("dune-mmesh"):
        print("mmesh is cloned")
        git("clone", "https://gitlab.dune-project.org/samuel.burbulla/dune-mmesh.git", "-b", "release/1.1")

    if  DOWNLOAD_ONLY:
        return

    if  (CLEANUP and os.path.exists("dune-mmesh")):
        shutil.rmtree('dune-mmesh')
        print("mmesh is removed")

def installFunctions():
    os.chdir(TOPDIR)
    if not checkLocationForDuneModules():
        return
    if not os.path.exists("dune-functions"):
        print("functions is cloned")
        git("clone", "https://gitlab.dune-project.org/staging/dune-functions.git", "-b", "releases/" + DUNE_VERSION)

    if  DOWNLOAD_ONLY:
        return

    if  (CLEANUP and os.path.exists("dune-functions")):
        shutil.rmtree('dune-functions')
        print("dune-functions is removed")

def git(*args):
    return subprocess.check_call(['git'] + list(args))

def usage():
    print( "")
    print( "Usage: ./installexternal.py [OPTIONS] PACKAGES")
    print( "")
    print( "Where PACKAGES is one or more of the following")
    print( "Multiple packages can be downloaded as a group:")
    print( "  all               Install everything and the kitchen sink.")
    print( "  dumux-extensions  Download dumux-course and dumux-lecture.")
    print( "  dune-extensions   Download dune-uggrid, dune-alugrid, dune-foamgrid,")
    print( "                    dune-subgrid, dune-spgrid, dune-mmesh and dune-functions.")
    print( "  optimization      Download and install glpk and nlopt.")
    print( "  others            Download and install opm , metis and gstat.")
    print( "")
    print( "... or individually as:")
    print( "  lecture           Download dumux-lecture.")
    print( "  course            Download dumux-course.")

    print( "")
    print( "  ug                Download dune-uggrid.")
    print( "  alugrid           Download dune-alugrid.")
    print( "  foamgrid          Download dune-foamgrid.")
    print( "  subgrid           Download dune-subgrid.")
    print( "  spgrid            Download dune-spgrid.")
    print( "  mmesh             Download dune-mmesh.")
    print( "  functions         Download dune-functions.")

    print( "")
    print( "  glpk             Download and install glpk.")
    print( "  nlopt            Download and install nlopt.")

    print( "")
    print( "  opm              Download opm modules required for cornerpoint grids.")
    print( "  metis            Install the METIS graph partitioner.")
    print( "  gstat            Download and install gstat.")

    print( "")
    print( "The following options are recoginzed:")
    print( "    --parallel       Enable parallelization if available.")
    print( "    --debug          Compile with debugging symbols and without optimization.")
    print( "    --clean          Delete all files for the given packages.")
    print( "    --download       Only download the packages.")


def installDumuxExtensions():
    installLecture()
    installCourse()

def installDuneExtensions():
    installUG()
    installAluGrid()
    installFoamGrid()
    installSubGrid()
    installSpGrid()
    installMmesh()
    installFunctions()


def installOptimizationStuff():
    createExternalDirectory()
    installGLPK()
    installNLOPT()

def installOthers():
    createExternalDirectory()
    installOPM()
    installMETIS()
    installGStat()


if __name__ == "__main__":

    CORRECT_LOCATION_FOR_DUNE_MODULES = False
    ENABLE_MPI = False
    ENABLE_DEBUG = False
    ENABLE_PARALLEL = False
    CLEANUP = False
    DOWNLOAD_ONLY = False
    DUNE_VERSION="2.7"
    DUMUX_VERSION="3.2"

    TOPDIR = os.getcwd()
    EXTDIR = os.getcwd() + "/external"

    if len(sys.argv)==1 or sys.argv[1] == "-h":
        usage()
        sys.exit()

    arguments = sys.argv[1:]
    # options are set
    if("--debug" in arguments):
        ENABLE_DEBUG = True
    if("--clean" in arguments):
        CLEANUP = True
    if("--download" in arguments):
        DOWNLOAD_ONLY = True
    if("--parallel" in arguments):
        ENABLE_PARALLEL = True
        ENABLE_MPI = True
        MPICC = subprocess.check_output(["which", "mpicc"]).decode().strip()
        MPICXX = subprocess.check_output(["which", "mpicxx"]).decode().strip()
        MPIF77 = subprocess.check_output(["which", "mpif77"]).decode().strip()

    # Downloads and installations
    # group installation
    if("all" in arguments):
        print("All external modules are installed")
        installDumuxExtensions()
        installDuneExtensions()
        installOptimizationStuff()
        installOthers()

        sys.exit()

    if("dumux-extensions" in arguments):
        print("dumux-extensions are downloaded, this includes: dumux-course and dumux-lecture")
        installDumuxExtensions()
    if("dune-extensions" in arguments):
        print("dune-extensions are downloaded, this includes: dune-uggrid, dune-alugrid, dune-foamgrid, dune-subgrid, dune-spgrid and dune-mesh")
        installDuneExtensions()
    if("optimization" in arguments):
        print("libraries for solving optimization problems are installed, this includes: glpk and nlopt")
        installOptimizationStuff()
    if("others" in arguments):
        print("other libraries are installed, this includes: opm, metis and gstat")
        installOthers()

    # single modules/packages installations
    if("lecture" in arguments):
        print("dumux-lecture is installed")
        installLecture()
    if("course" in arguments):
        print("dumux-course is installed")
        installCourse()
    if("ug" in arguments):
        print("UG grid library is installed")
        installUG()
    if("alugrid" in arguments):
        print( " dune-alugrid is downloaded")
        installAluGrid()
    if("foamgrid" in arguments):
        print( " dune-foam is downloaded")
        installFoamGrid()
    if("subgrid" in arguments):
        print( " dune-subgrid is downloaded")
        installSubGrid()
    if("spgrid" in arguments):
        print( " dune-spgrid is downloaded")
        installSpGrid()
    if("mmesh" in arguments):
        print( " dune-mmesh is downloaded")
        installMmesh()
    if("functions" in arguments):
        print( " dune-functions is downloaded")
        installFunctions()
    if("glpk" in arguments):
        print( " glpk is is installed")
        createExternalDirectory()
        installGLPK()
    if("nlopt" in arguments):
        print( " nlopt is is installed")
        createExternalDirectory()
        installNLOPT()
    if("opm" in arguments):
        print( " opm modules are downloaded")
        createExternalDirectory()
        installOPM()
    if("metis" in arguments):
        print( " metis is installed")
        createExternalDirectory()
        installMETIS()
    if("gstat" in arguments):
        print( " gstat is installed")
        createExternalDirectory()
        installGStat()