import os
import subprocess
import platform

from SetupPython import PythonConfiguration as PythonRequirements

# Make sure everything we need for the setup is installed
PythonRequirements.Validate()

from SetupCmake import CmakeConfiguration as CmakeRequirements
from SetupVulkan import VulkanConfiguration as VulkanRequirements
os.chdir('../') # Change from devtools/scripts directory to root

cmakeInstalled = CmakeRequirements.Validate()
VulkanRequirements.Validate()

print("\nUpdating submodules...")
subprocess.call(["git", "submodule", "update", "--init", "--recursive"])

vkPath = VulkanRequirements.GetVulkanSDK()

print("\nSetting up .env.cmake file")
envFile = open(".env.cmake", "w")
envFile.write(f"set(GLFW_PATH ./GearHead-Engine/vendor/GLFW)\nset(GLM_PATH ./GearHead-Engine/vendor/GLM)\nset(VULKAN_SDK_PATH {vkPath})\nset(TINYOBJ_PATH ./GearHead-Engine/vendor/tinyobjloader)\n")


if (cmakeInstalled):
    #if platform.system() == "Windows":
    #    print("\nRunning premake...")
    #    subprocess.call([os.path.abspath("./scripts/Win-GenProjects.bat"), "nopause"])

    print("\nSetup completed!")
else:
    print("GearHead requires Cmake to generate project files.")