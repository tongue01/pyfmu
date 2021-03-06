#pragma once
/**
 * Provides functionality related to finding and instantiating example projects
*/
#include <filesystem>

#include <tmpdir.hpp>

/**
 * Defines the directory in which example projects are located.
 * Due to the fact that C++ is compiled, it has no notion of 'where' it is running from. 
 * Therefor it is not possible to define a path realtive to say the 'example_finder.cpp' file.
*/
void setProjecsDirectory(std::filesystem::path path);

/**
 * Returns the path to the resources folder of an example project
*/
std::string get_resource_uri(std::string example_name);

/**
 * Defines the path to the tool export Python FMUs, py2fmu.py
*/
void setBuilderPath(std::filesystem::path path);

class ExampleArchive
{
public:
    explicit ExampleArchive(std::string exampleName);

    std::filesystem::path getRoot();
    std::filesystem::path getResources();
    std::string getResourcesURI();

private:
    TmpDir td;
    std::string exampleName;
};