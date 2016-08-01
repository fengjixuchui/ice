// **********************************************************************
//
// Copyright (c) 2003-2015 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Ice/Config.h>

#include <IceUtil/IceUtil.h>
#include <Ice/StringConverter.h>
#include <Ice/IconvStringConverter.h>
#include <IceUtil/StringUtil.h>

#include <Ice/Communicator.h>
#include <Ice/Initialize.h>
#include <Ice/LocalException.h>
#include <Ice/LoggerUtil.h>

using namespace IceUtilInternal;
using namespace Ice;
using namespace std;

class ICE_API StringConverterPlugin : public Plugin
{
public:

    StringConverterPlugin(const CommunicatorPtr&, 
                          const IceUtil::StringConverterPtr&, const IceUtil::WstringConverterPtr& = 0);
    
    virtual void initialize();
    
    virtual void destroy();
};

StringConverterPlugin::StringConverterPlugin(const CommunicatorPtr& /*notused*/, 
                                             const IceUtil::StringConverterPtr& stringConverter, 
                                             const IceUtil::WstringConverterPtr& wstringConverter) 
{
    IceUtil::setProcessStringConverter(stringConverter);
    IceUtil::setProcessWstringConverter(wstringConverter);
}
    
void StringConverterPlugin::initialize()
{
    // no op
}
    
void StringConverterPlugin::destroy()
{
    // no op
}

//
// The entry point for the string converter plugin built-in the Ice library
//
extern "C"
{

ICE_API Plugin*
createStringConverter(const CommunicatorPtr& communicator, const string& name, const StringSeq& args)
{
    IceUtil::StringConverterPtr stringConverter;
    IceUtil::WstringConverterPtr wstringConverter;

    if(args.size() > 2)
    {
        Error out(communicator->getLogger());
        out << "Plugin " << name << ": too many arguments";
        return 0;
    }

    try
    {

#ifdef _WIN32
        int cp = -1;

        for(size_t i = 0; i < args.size(); ++i)
        {
            if(args[i].find("windows=") == 0)
            {
                cp = atoi(args[i].substr(strlen("windows=")).c_str());
            }
            else if(args[i].find("iconv=") != 0)
            {
                Error out(communicator->getLogger());
                out << "Plugin " << name << ": invalid \"" << args[i] << "\" argument";
                return 0;
            }
        }

        if(cp == -1)
        {
            Error out(communicator->getLogger());
            out << "Plugin " << name << ": missing windows=<code page> argument";
            return 0;
        }

        if(cp == 0 || cp == INT_MAX || cp < 0)
        {
            Error out(communicator->getLogger());
            out << "Plugin " << name << ": invalid Windows code page";
            return 0;
        }

        stringConverter = createWindowsStringConverter(static_cast<unsigned int>(cp));
#else
        StringSeq iconvArgs;

        for(size_t i = 0; i < args.size(); ++i)
        {
            if(args[i].find("iconv=") == 0)
            {
                if(!IceUtilInternal::splitString(args[i].substr(strlen("iconv=")), ", \t\r\n", iconvArgs))
                {
                    Error out(communicator->getLogger());
                    out << "Plugin " << name << ": invalid iconv argument";
                    return 0;
                }
            }
            else if(args[i].find("windows=") != 0)
            {
                Error out(communicator->getLogger());
                out << "Plugin " << name << ": invalid \"" << args[i] << "\" argument";
                return 0;
            }
        }

        switch(iconvArgs.size())
        {
            case 0:
            {
                stringConverter = createIconvStringConverter<char>("");
                break;
            }
            case 1:
            {
                stringConverter = createIconvStringConverter<char>(iconvArgs[0].c_str());
                break;
            }
            case 2:
            {
                stringConverter = createIconvStringConverter<char>(iconvArgs[0].c_str());
                wstringConverter = createIconvStringConverter<wchar_t>(iconvArgs[1].c_str());
                break;
            }
            default:
            {
                assert(0);
            }
        }

#endif

        return new StringConverterPlugin(communicator, stringConverter, wstringConverter);
    }
    catch(const std::exception& ex)
    {
        Error out(communicator->getLogger());
        out << "Plugin " << name << ": creation failed with " << ex.what();
        return 0;
    }
    catch(...)
    {
        Error out(communicator->getLogger());
        out << "Plugin " << name << ": creation failed with unknown exception";
        return 0;
    }
}

}

namespace Ice
{

ICE_API void
registerIceStringConverter(bool loadOnInitialize)
{
    Ice::registerPluginFactory("IceStringConverter", createStringConverter, loadOnInitialize);
}

}