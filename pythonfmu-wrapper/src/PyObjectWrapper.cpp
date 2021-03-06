#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "fmt/format.h"

#include "fmi/fmi2TypesPlatform.h"
#include "pythonfmu/PyConfiguration.hpp"
#include <pythonfmu/PyException.hpp>
#include <pythonfmu/PyObjectWrapper.hpp>
#include "utility/py_compatability.hpp"

using namespace fmt;
using namespace pyconfiguration;
using namespace std;
using namespace filesystem;

namespace pythonfmu
{

/**
 * @brief Appends path of resources folder to the Python interpreter path. This
 * allows the interpreter to locate and load the main script.
 *
 * @param resource_path path to the resources dir supplied when FMU is
 * initialized
 */
void append_resources_folder_to_python_path(path &resource_path)
{

  ostringstream oss;
  oss << "import sys\n";
  oss << "sys.path.append(r'" << resource_path.string() << "')\n";

  string str = oss.str();
  const char *cstr = str.c_str();

  int err = PyCompat::PyRun_SimpleString(cstr);


  if (err != 0)
    throw runtime_error("Failed to append folder to python path\n");
}

void PyObjectWrapper::instantiate_main_class(string module_name,
                                             string main_class)
{

  logger->ok(format("importing Python module: {}, into the interpreter\n", module_name));

  pModule_ = PyImport_ImportModule(module_name.c_str());

  if (pModule_ == NULL)
  {

    auto pyErr = get_py_exception();
    auto msg = format("module could not be imported. Ensure that main script "
                      "defined inside the wrapper configuration matches a "
                      "Python script. Error from python was:\n{}",
                      pyErr);
    logger->fatal(msg);
    throw runtime_error(msg);
  }

  logger->ok(format("module: {} was successfully imported\n", module_name));

  logger->ok(format("attempting to read definition of main class: {} from the imported module\n", main_class));

  pClass_ = PyObject_GetAttrString(pModule_, main_class.c_str());

  if (pClass_ == nullptr)
  {
    auto pyErr = get_py_exception();
    auto msg = format("Python module: {} was successfully loaded, but the defintion "
                      "of the main class {} could not be loaded. Ensure that the "
                      "specified module contains a definition of the class. Python error was:\n{}\n",
                      module_name, main_class, pyErr);
    logger->fatal(msg);
    throw runtime_error(msg);
  }

  logger->ok(format("class definition of {} was successfully read\n", main_class));

  logger->ok(format("attempting to construct an instance of the class: {}\n", main_class));

  pInstance_ = PyObject_CallFunctionObjArgs(pClass_, nullptr);

  if (pInstance_ == nullptr)
  {
    auto pyErr = get_py_exception();
    auto msg = format("Failed to instantiate class: {}, ensure that the Python script is valid and that defines a parameterless constructor. Python error was:\n{}\n", main_class, pyErr);
    logger->fatal(msg);
    throw runtime_error(msg);
  }

  propagate_python_log_messages();
  this->logger->ok(format("Sucessfully created an instance of class: {} defined in module: {}\n", main_class, module_name));
}

PyObjectWrapper::PyObjectWrapper(path resource_path, Logger *logger) : logger(logger)
{

  PyGIL g;

  if (!Py_IsInitialized())
  {
    throw runtime_error(
        "The Python object cannot be instantiated due to the python intrepeter "
        "not being instantiated. Ensure that Py_Initialize() is invoked "
        "successfully prior to the invoking the constructor.");
  }

  logger->ok(format("appending path of resource folder to Python "
                    "Interpreter, the specified path is: {}\n",
                    resource_path.string()));

  append_resources_folder_to_python_path(resource_path);

  logger->ok(format("Successfully appended resources directory to Python path\n"));

  auto config_path = resource_path / "slave_configuration.json";

  logger->ok(format("Reading configuration file located at: {}\n", config_path.string()));

  try
  {
    auto config = read_configuration(config_path, logger);

    logger->ok(format("successfully read configuration file, specifying the following: main "
                      "script is: {} and main class is: {}\n",
                      config.main_script, config.main_class));

    string module_name =
        (path(config.main_script).filename().replace_extension("")).string();

    instantiate_main_class(module_name, config.main_class);
  }
  catch (const exception &e)
  {
    logger->error(format("Failed to read configuration file, an expection was thrown:\n{}", e.what()));
    throw;
  }
}

PyObjectWrapper::PyObjectWrapper(PyObjectWrapper &&other) : pModule_(other.pModule_), pClass_(other.pClass_), pInstance_(other.pInstance_), logger(std::move(other.logger))
{
}

void PyObjectWrapper::setupExperiment(double startTime)
{
  PyGIL g;
  auto f =
      PyObject_CallMethod(pInstance_, "setup_experiment", "(d)", startTime);
      propagate_python_log_messages();
  if (f == nullptr)
  {
    handle_py_exception();
  }
  Py_DECREF(f);
}

void PyObjectWrapper::enterInitializationMode()
{

  PyGIL g;
  auto f =
      PyObject_CallMethod(pInstance_, "enter_initialization_mode", nullptr);
  if (f == nullptr)
  {
    handle_py_exception();
  }
  Py_DECREF(f);
}

void PyObjectWrapper::exitInitializationMode()
{
  PyGIL g;

  auto f = PyObject_CallMethod(pInstance_, "exit_initialization_mode", nullptr);
  if (f == nullptr)
  {
    handle_py_exception();
  }
  Py_DECREF(f);
}

bool PyObjectWrapper::doStep(double currentTime, double stepSize)
{
  PyGIL g;

  auto f = PyObject_CallMethod(pInstance_, "do_step", "(dd)", currentTime, stepSize);

  if (f == nullptr)
  {
    std::string err = get_py_exception();
    logger->error(format("FMI2 do step failed due to Python error:\n{}",err));
  }

  propagate_python_log_messages();

  bool status = static_cast<bool>(PyObject_IsTrue(f));
  Py_DECREF(f);
  return status;
}

void PyObjectWrapper::reset()
{
  PyGIL g;

  auto f = PyObject_CallMethod(pInstance_, "reset", nullptr);
  if (f == nullptr)
  {
    handle_py_exception();
  }
  Py_DECREF(f);
}

void PyObjectWrapper::terminate()
{
  PyGIL g;

  auto f = PyObject_CallMethod(pInstance_, "terminate", nullptr);
  if (f == nullptr)
  {
    handle_py_exception();
  }
  Py_DECREF(f);
}

void PyObjectWrapper::getInteger(const fmi2ValueReference *vr, std::size_t nvr,
                                 fmi2Integer *values) const
{
  PyGIL g;

  PyObject *vrs = PyList_New(nvr);
  PyObject *refs = PyList_New(nvr);
  for (int i = 0; i < nvr; i++)
  {
    PyList_SetItem(vrs, i, Py_BuildValue("i", vr[i]));
    PyList_SetItem(refs, i, Py_BuildValue("i", 0));
  }
  auto f =
      PyObject_CallMethod(pInstance_, "__get_integer__", "(OO)", vrs, refs);
  Py_DECREF(vrs);
  if (f == nullptr)
  {
    handle_py_exception();
  }
  Py_DECREF(f);

  for (int i = 0; i < nvr; i++)
  {
    PyObject *value = PyList_GetItem(refs, i);
    values[i] = static_cast<int>(PyLong_AsLong(value));
  }

  Py_DECREF(refs);
}

void PyObjectWrapper::getReal(const fmi2ValueReference *vr, std::size_t nvr,
                              fmi2Real *values) const
{
  PyGIL g;

  PyObject *vrs = PyList_New(nvr);
  PyObject *refs = PyList_New(nvr);
  for (int i = 0; i < nvr; i++)
  {
    PyList_SetItem(vrs, i, Py_BuildValue("i", vr[i]));
    PyList_SetItem(refs, i, Py_BuildValue("d", 0.0));
  }

  auto f = PyObject_CallMethod(pInstance_, "__get_real__", "(OO)", vrs, refs);
  Py_DECREF(vrs);
  propagate_python_log_messages();
  
  bool call_failed = (f == nullptr);

  

  if (!call_failed)
  {
      Py_DECREF(f);

    for (int i = 0; i < nvr; i++)
    {
      PyObject *value = PyList_GetItem(refs, i);
      values[i] = PyFloat_AsDouble(value);
    }
  }
  else
  {
    std::string py_err_msg = get_py_exception();
    logger->error(py_err_msg);
  }

  Py_DECREF(refs);
}

void PyObjectWrapper::getBoolean(const fmi2ValueReference *vr, std::size_t nvr,
                                 fmi2Boolean *values) const
{
  PyGIL g;

  PyObject *vrs = PyList_New(nvr);
  PyObject *refs = PyList_New(nvr);
  for (int i = 0; i < nvr; i++)
  {
    PyList_SetItem(vrs, i, Py_BuildValue("i", vr[i]));
    PyList_SetItem(refs, i, Py_BuildValue("i", 0));
  }
  auto f =
      PyObject_CallMethod(pInstance_, "__get_boolean__", "(OO)", vrs, refs);
  Py_DECREF(vrs);
  if (f == nullptr)
  {
    handle_py_exception();
  }
  Py_DECREF(f);

  for (int i = 0; i < nvr; i++)
  {
    PyObject *value = PyList_GetItem(refs, i);
    values[i] = PyObject_IsTrue(value);
  }

  Py_DECREF(refs);
}

void PyObjectWrapper::getString(const fmi2ValueReference *vr, std::size_t nvr,
                                fmi2String *values) const
{
  PyGIL g;

  PyObject *vrs = PyList_New(nvr);
  PyObject *refs = PyList_New(nvr);
  for (int i = 0; i < nvr; i++)
  {
    PyList_SetItem(vrs, i, Py_BuildValue("i", vr[i]));
    PyList_SetItem(refs, i, Py_BuildValue("s", ""));
  }
  auto f = PyObject_CallMethod(pInstance_, "__get_string__", "(OO)", vrs, refs);
  Py_DECREF(vrs);
  if (f == nullptr)
  {
    handle_py_exception();
  }
  Py_DECREF(f);

  for (int i = 0; i < nvr; i++)
  {
    PyObject *value = PyList_GetItem(refs, i);
    values[i] = PyCompat::PyUnicode_AsUTF8(value);
  }

  Py_DECREF(refs);
}

fmi2Status PyObjectWrapper::setDebugLogging(bool loggingOn, size_t nCategories, const char* const categories[]) const
{
  auto py_categories = PyList_New(nCategories);

  for(int i = 0; i < nCategories; ++i)
  {
    PyList_SetItem(py_categories,i,Py_BuildValue("s", categories[i]));
  }

  auto f = PyObject_CallMethod(pInstance_,"__set_debug_logging__","(iO)", loggingOn, py_categories);
  Py_DECREF(py_categories);
  
  if(f == nullptr)
  {
    auto msg = get_py_exception();
    logger->error(format("Failed to set debug logging categories, due to python error:\n{}",msg));
    return fmi2Error;
  }
  propagate_python_log_messages();

  Py_DECREF(f);

  return fmi2OK;
}

void PyObjectWrapper::setInteger(const fmi2ValueReference *vr, std::size_t nvr,
                                 const fmi2Integer *values)
{
  PyGIL g;

  PyObject *vrs = PyList_New(nvr);
  PyObject *refs = PyList_New(nvr);
  for (int i = 0; i < nvr; i++)
  {
    PyList_SetItem(vrs, i, Py_BuildValue("i", vr[i]));
    PyList_SetItem(refs, i, Py_BuildValue("i", values[i]));
  }

  auto f =
      PyObject_CallMethod(pInstance_, "__set_integer__", "(OO)", vrs, refs);
  Py_DECREF(vrs);
  Py_DECREF(refs);

  if (f == nullptr)
  {
    handle_py_exception();
  }

  Py_DECREF(f);
}

void PyObjectWrapper::setReal(const fmi2ValueReference *vr, std::size_t nvr,
                              const fmi2Real *values)
{
  PyGIL g;

  PyObject *vrs = PyList_New(nvr);
  PyObject *refs = PyList_New(nvr);
  for (int i = 0; i < nvr; i++)
  {
    PyList_SetItem(vrs, i, Py_BuildValue("i", vr[i]));
    PyList_SetItem(refs, i, Py_BuildValue("d", values[i]));
  }

  auto f = PyObject_CallMethod(pInstance_, "__set_real__", "(OO)", vrs, refs);
  Py_DECREF(vrs);
  Py_DECREF(refs);

  if (f == nullptr)
  {
    auto err = get_py_exception();
    ostringstream oss;
    oss << "Failed to set real values of the python instance. Ensure that the "
           "script implements the '__set__real__' method correctly, for "
           "example by inheriting from 'FMI2Slave'.\n"
        << "Python error is:\n"
        << err;
    throw runtime_error(oss.str());
  }

  Py_DECREF(f);
}

void PyObjectWrapper::setBoolean(const fmi2ValueReference *vr, std::size_t nvr,
                                 const fmi2Boolean *values)
{
  PyGIL g;

  PyObject *vrs = PyList_New(nvr);
  PyObject *refs = PyList_New(nvr);
  for (int i = 0; i < nvr; i++)
  {
    PyList_SetItem(vrs, i, Py_BuildValue("i", vr[i]));
    PyList_SetItem(refs, i, PyBool_FromLong(values[i]));
  }

  auto f =
      PyObject_CallMethod(pInstance_, "__set_boolean__", "(OO)", vrs, refs);
  Py_DECREF(vrs);
  Py_DECREF(refs);
  if (f == nullptr)
  {
    handle_py_exception();
  }
  Py_DECREF(f);
}

void PyObjectWrapper::setString(const fmi2ValueReference *vr, std::size_t nvr,
                                const fmi2String *value)
{
  PyGIL g;

  PyObject *vrs = PyList_New(nvr);
  PyObject *refs = PyList_New(nvr);
  for (int i = 0; i < nvr; i++)
  {
    PyList_SetItem(vrs, i, Py_BuildValue("i", vr[i]));
    PyList_SetItem(refs, i, Py_BuildValue("s", value[i]));
  }

  auto f = PyObject_CallMethod(pInstance_, "__set_string__", "(OO)", vrs, refs);
  Py_DECREF(vrs);
  Py_DECREF(refs);
  if (f == nullptr)
  {
    handle_py_exception();
  }
  Py_DECREF(f);
}

PyObjectWrapper::~PyObjectWrapper()
{
  PyGIL g;

  Py_XDECREF(pInstance_);
  Py_XDECREF(pClass_);
  Py_XDECREF(pModule_);
}

PyObjectWrapper &PyObjectWrapper::operator=(PyObjectWrapper &&other)
{
  this->pClass_ = other.pClass_;
  this->pModule_ = other.pModule_;
  this->pInstance_ = other.pInstance_;
  this->logger = move(other.logger);
  return *this;
}

void PyObjectWrapper::propagate_python_log_messages() const
{
  PyGIL g;

  auto f = PyObject_CallMethod(pInstance_, "__get_log_size__", "()");

  if (f == nullptr)
  { 
    std::string py_err_msg = get_py_exception();
    logger->error(format("Failed to read log messages from the Python instance. Call to __get_log_size__ failed due to:\n{}", py_err_msg));
    return;
  }
  Py_DECREF(f);
  long n_messages = PyLong_AsLong(f);
  

  bool failed_to_parse = (n_messages == -1);
  if(failed_to_parse)
  {
    std::string py_err_msg = get_py_exception();
    logger->error(format("Failed to read log messages from the Python instance. Call to __get_log_size__ returned invalid type:\n{}", py_err_msg));
    return;
  }

  if(n_messages == 0)
    return;
  
  f = PyObject_CallMethod(pInstance_,"__pop_log_messages__","(i)",n_messages);

  for(int i = 0; i < n_messages; ++i)
  {
    PyObject *value = PyList_GetItem(f, i);
    
    if(value == nullptr)
    {
      logger->warning("Failed to parse read log message");
      return;
    }

    PyObject* py_status = PyTuple_GetItem(value,0);
    PyObject* py_category = PyTuple_GetItem(value,1);
    PyObject* py_message = PyTuple_GetItem(value,2);

    if(py_status == nullptr || py_category == nullptr || py_message == nullptr)
    {
      auto msg = "Failed to read log messages, unable to unpack message tuples";
      logger->warning(msg);
    }
    fmi2Status status = (fmi2Status)(PyLong_AsLong(py_status));
    const char* category = PyCompat::PyUnicode_AsUTF8(py_category);
    const char* message = PyCompat::PyUnicode_AsUTF8(py_message);

    logger->log(status,category,message);
  }
   
    

  
}

} // namespace pythonfmu
