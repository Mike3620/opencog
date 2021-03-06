// Enables support for loading MindAgents and CogServer requests written in
// Python
//
#ifndef _OPENCOG_PYTHON_MODULE_H
#define _OPENCOG_PYTHON_MODULE_H

#include "PyIncludeWrapper.h"

#include <string>

#include <opencog/server/Agent.h>
#include <opencog/server/Factory.h>
#include <opencog/server/Module.h>
#include <opencog/server/Request.h>
#include <opencog/server/CogServer.h>
#include <opencog/cython/PythonEval.h>

#include "PyMindAgent.h"
#include "PyRequest.h"
#include "agent_finder_types.h"

namespace opencog
{

class CogServer;

class PythonAgentFactory : public AbstractFactory<Agent>
{
    // Store the name of the python module and class so that we can instantiate
    // them
    std::string _pySrcModuleName;
    std::string _pyClassName;
    ClassInfo* _ci;
public:
    explicit PythonAgentFactory(std::string& module, std::string& clazz)
      : AbstractFactory<Agent>()
    {
        _pySrcModuleName = module;
        _pyClassName = clazz;
        _ci = new ClassInfo("opencog::PyMindAgent(" + module + "." + clazz + ")");
    }
    virtual ~PythonAgentFactory()
    {
        delete _ci;
    }
    virtual Agent* create() const;
    virtual const ClassInfo& info() const { return *_ci; }
};

class PythonRequestFactory : public AbstractFactory<Request>
{
    // Store the name of the python module and class so that we can instantiate
    // them
    std::string _pySrcModuleName;
    std::string _pyClassName;
    RequestClassInfo* _cci;
public:
    explicit PythonRequestFactory(std::string& module, const std::string& clazz, 
                                  const std::string& short_desc, 
                                  const std::string& long_desc,
                                  bool is_shell)
       : AbstractFactory<Request>()
    {
        _pySrcModuleName = module;
        _pyClassName = clazz;
        _cci = new RequestClassInfo(
              _pySrcModuleName + _pyClassName, short_desc, long_desc, is_shell);
    }
    virtual ~PythonRequestFactory()
    {
        delete _cci;
    }
    virtual Request* create() const;
    virtual const ClassInfo& info() const { return *_cci; }
};

class PythonModule : public Module
{
    DECLARE_CMD_REQUEST(PythonModule, "loadpy", do_load_py,
       "Load commands and MindAgents from a python module",
       "Usage: load_py file_name\n\n"
       "Load commands and MindAgents, written in python, from a file."
       "After loading, commands will appear in the list of available "
       "commands (use 'h' to list).  Commands must be implemented as "
       "python modules, inheritiing from the class opencog.cogserver.Request. "
       "Likewise, mind agents must inherit from opencog.cogserver.MindAgent. "
       "The loaded agents can be viewed with the 'agents-list' command.",
       false, false);

private:

    std::vector<std::string> _agentNames;
    std::vector<std::string> _requestNames;

    // Main thread state only.
    PyThreadState* _mainstate;

    bool preloadModules();
    bool unregisterAgentsAndRequests();
public:

    virtual const ClassInfo& classinfo() const { return info(); }
    static const ClassInfo& info() {
        static const ClassInfo _ci("opencog::PythonModule");
        return _ci;
    }
    static inline const char* id();

    PythonModule();
    ~PythonModule();
    void init();

}; // class

} // namespace opencog

#endif // _OPENCOG_SINGLE_AGENT_MODULE_H

