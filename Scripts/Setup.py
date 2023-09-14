import os
import subprocess
import platform
import pathlib
import shutil as sh
import sys


from SetupPython import PythonConfiguration as PythonRequirements

# Make sure everything we need for the setup is installed
PythonRequirements.Validate()

from SetupCmake import CmakeConfiguration as CmakeRequirements
from SetupVulkan import VulkanConfiguration as VulkanRequirements
from SetupPremake import PremakeConfiguration as PremakeRequirements
os.chdir('../') # Change from devtools/scripts directory to root

currentPath = str(pathlib.Path().resolve()).replace('\\', '/')  
cmakeInstalled = CmakeRequirements.Validate()
VulkanRequirements.Validate()

print("\nUpdating submodules...")

try:
    out = subprocess.check_output(["git", "submodule", "update", "--init", "--recursive"]).decode()
except FileNotFoundError:
    print("requires git to run")
    raise FileNotFoundError
except subprocess.CalledProcessError as e:
    if(e.returncode == 128):
        print("Repository was not cloned, cloning necessary modules")
        #need to clone the directories because SOMEONE DOWLOADED IT AS A ZIP
        subprocess.call(["git", "clone", "https://github.com/glfw/glfw.git",    currentPath + "/GearHead-Engine/vendor/GLFW"])
        subprocess.call(["git", "clone", "https://github.com/g-truc/glm.git",   currentPath + "GearHead-Engine/vendor/GLM"])
        subprocess.call(["git", "clone", "https://github.com/gabime/spdlog",    currentPath + "GearHead-Engine/vendor/spdlog"])
        subprocess.call(["git", "clone", "https://github.com/ocornut/imgui",    currentPath + "GearHead-Engine/vendor/ImGUI"])


vkPath = VulkanRequirements.GetVulkanSDK()




#if (cmakeInstalled and premakeInstalled):
if (cmakeInstalled):
    if CmakeRequirements.UseExternal():
        subprocess.call([os.path.abspath("./scripts/Compile.bat")])

    subprocess.call([currentPath + "/vendor/premake/bin/premake5.exe", "vs2022"])
    

else:
    print("GearHead requires Cmake and Premake to generate Project.")
