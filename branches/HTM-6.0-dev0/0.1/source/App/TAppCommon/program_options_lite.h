
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <stdlib.h>
#include <errno.h>
#include <cstring>
#include <math.h>


#ifdef WIN32
#define strdup _strdup
#endif

namespace df
{
  namespace program_options_lite
  {
    struct Options;
    
    void doHelp(std::ostream& out, Options& opts, unsigned columns = 80);
    unsigned parseGNU(Options& opts, unsigned argc, const char* argv[]);
    unsigned parseSHORT(Options& opts, unsigned argc, const char* argv[]);
    std::list<const char*> scanArgv(Options& opts, unsigned argc, const char* argv[]);
    void scanLine(Options& opts, std::string& line);
    void scanFile(Options& opts, std::istream& in);
    void setDefaults(Options& opts);
    void parseConfigFile(Options& opts, const std::string& filename);
    bool storePair(Options& opts, const std::string& name, const std::string& value);
    
    /** OptionBase: Virtual base class for storing information relating to a
     * specific option This base class describes common elements.  Type specific
     * information should be stored in a derived class. */
    struct OptionBase
    {
      OptionBase(const std::string& name, const std::string& desc)
      : opt_string(name), opt_desc(desc)
      {};
      
      virtual ~OptionBase() {}
      
      /* parse argument @arg, to obtain a value for the option */
      virtual void parse(const std::string& arg) = 0;
      /* set the argument to the default value */
      virtual void setDefault() = 0;
      
      std::string opt_string;
      std::string opt_desc;
    };
    
    /** Type specific option storage */
    template<typename T>
    struct Option : public OptionBase
    {
      Option(const std::string& name, T& storage, T default_val, const std::string& desc)
      : OptionBase(name, desc), opt_storage(storage), opt_default_val(default_val)
      {}
      
      void parse(const std::string& arg);
      
      void setDefault()
      {
        opt_storage = opt_default_val;
      }
      
      T& opt_storage;
      T opt_default_val;
    };
    
    /* Generic parsing */
    template<typename T>
    inline void
    Option<T>::parse(const std::string& arg)
    {
      std::istringstream arg_ss (arg,std::istringstream::in);
      arg_ss >> opt_storage;
    }
    
    /* string parsing is specialized -- copy the whole string, not just the
     * first word */
    template<>
    inline void
    Option<std::string>::parse(const std::string& arg)
    {
      opt_storage = arg;
    }
    
    template<>
    inline void
      Option<char*>::parse(const std::string& arg)
    {
      opt_storage = arg.empty() ? NULL : strdup(arg.c_str()) ;
    }

    template<>
    inline void
      Option< std::vector<char*> >::parse(const std::string& arg)
    {
      opt_storage.clear(); 
      
      char* pcStart = (char*) arg.data();      
      char* pcEnd = strtok (pcStart," ");

      while (pcEnd != NULL)
      {
        size_t uiStringLength = pcEnd - pcStart;
        char* pcNewStr = (char*) malloc( uiStringLength + 1 );
        strncpy( pcNewStr, pcStart, uiStringLength); 
        pcNewStr[uiStringLength] = '\0'; 
        pcStart = pcEnd+1; 
        pcEnd = strtok (NULL, " ,.-");
        opt_storage.push_back( pcNewStr ); 
      }      
    }


    template<>    
    inline void
      Option< std::vector<double> >::parse(const std::string& arg)
    {
      char* pcNextStart = (char*) arg.data();
      char* pcEnd = pcNextStart + arg.length();

      char* pcOldStart = 0; 

      size_t iIdx = 0; 

      while (pcNextStart < pcEnd)
      {
        errno = 0; 

        if ( iIdx < opt_storage.size() )
        {
          opt_storage[iIdx] = strtod(pcNextStart, &pcNextStart);
        }
        else
        {
        opt_storage.push_back( strtod(pcNextStart, &pcNextStart)) ;
        }
        iIdx++; 

        if ( errno == ERANGE || (pcNextStart == pcOldStart) )
        {
          std::cerr << "Error Parsing Doubles: `" << arg << "'" << std::endl;
          exit(EXIT_FAILURE);    
        };   
        while( (pcNextStart < pcEnd) && ( *pcNextStart == ' ' || *pcNextStart == '\t' || *pcNextStart == '\r' ) ) pcNextStart++;  
        pcOldStart = pcNextStart; 

      }
    }

    template<>
    inline void
      Option< std::vector<int> >::parse(const std::string& arg)
    {
      opt_storage.clear();


      char* pcNextStart = (char*) arg.data();
      char* pcEnd = pcNextStart + arg.length();

      char* pcOldStart = 0; 

      size_t iIdx = 0; 


      while (pcNextStart < pcEnd)
      {

        if ( iIdx < opt_storage.size() )
        {
          opt_storage[iIdx] = (int) strtol(pcNextStart, &pcNextStart,10);
        }
        else
        {
          opt_storage.push_back( (int) strtol(pcNextStart, &pcNextStart,10)) ;
        }
        iIdx++; 
        if ( errno == ERANGE || (pcNextStart == pcOldStart) )
        {
          std::cerr << "Error Parsing Integers: `" << arg << "'" << std::endl;
          exit(EXIT_FAILURE);
        };   
        while( (pcNextStart < pcEnd) && ( *pcNextStart == ' ' || *pcNextStart == '\t' || *pcNextStart == '\r' ) ) pcNextStart++;  
        pcOldStart = pcNextStart;
      }
    }


    template<>
    inline void
      Option< std::vector<bool> >::parse(const std::string& arg)
    {
      char* pcNextStart = (char*) arg.data();
      char* pcEnd = pcNextStart + arg.length();

      char* pcOldStart = 0; 

      size_t iIdx = 0; 

      while (pcNextStart < pcEnd)
      {
        if ( iIdx < opt_storage.size() )
        {
          opt_storage[iIdx] = (strtol(pcNextStart, &pcNextStart,10) != 0);
        }
        else
        {
          opt_storage.push_back(strtol(pcNextStart, &pcNextStart,10) != 0) ;
        }
        iIdx++; 

        if ( errno == ERANGE || (pcNextStart == pcOldStart) )
        {
          std::cerr << "Error Parsing Bools: `" << arg << "'" << std::endl;
          exit(EXIT_FAILURE);
        };   
        while( (pcNextStart < pcEnd) && ( *pcNextStart == ' ' || *pcNextStart == '\t' || *pcNextStart == '\r' ) ) pcNextStart++;  
        pcOldStart = pcNextStart;
      }
    }
    
    /** Option class for argument handling using a user provided function */
    struct OptionFunc : public OptionBase
    {
      typedef void (Func)(Options&, const std::string&);
      
      OptionFunc(const std::string& name, Options& parent_, Func *func_, const std::string& desc)
      : OptionBase(name, desc), parent(parent_), func(func_)
      {}
      
      void parse(const std::string& arg)
      {
        func(parent, arg);
      }
      
      void setDefault()
      {
        return;
      }
      
    private:
      Options& parent;
      void (*func)(Options&, const std::string&);
    };
    
    class OptionSpecific;
    struct Options
    {
      ~Options();
      
      OptionSpecific addOptions();
      
      struct Names
      {
        Names() : opt(0) {};
        ~Names()
        {
          if (opt)
            delete opt;
        }
        std::list<std::string> opt_long;
        std::list<std::string> opt_short;
        OptionBase* opt;
      };

      void addOption(OptionBase *opt);
      
      typedef std::list<Names*> NamesPtrList;
      NamesPtrList opt_list;
      
      typedef std::map<std::string, NamesPtrList> NamesMap;
      NamesMap opt_long_map;
      NamesMap opt_short_map;
    };
    
    /* Class with templated overloaded operator(), for use by Options::addOptions() */
    class OptionSpecific
    {
    public:
      OptionSpecific(Options& parent_) : parent(parent_) {}
      
      /**
       * Add option described by @name to the parent Options list,
       *   with @storage for the option's value
       *   with @default_val as the default value
       *   with @desc as an optional help description
       */
      template<typename T>
      OptionSpecific&
      operator()(const std::string& name, T& storage, T default_val, const std::string& desc = "")
      {
        parent.addOption(new Option<T>(name, storage, default_val, desc));
        return *this;
      }
      
      template<typename T>
      OptionSpecific&
        operator()(const std::string& name, std::vector<T>& storage, T default_val, unsigned uiMaxNum, const std::string& desc = "" )
      {
        std::string cNameBuffer;
        std::string cDescriptionBuffer;

        cNameBuffer       .resize( name.size() + 10 );
        cDescriptionBuffer.resize( desc.size() + 10 );

        storage.resize(uiMaxNum);
        for ( unsigned int uiK = 0; uiK < uiMaxNum; uiK++ )
        {
          // isn't there are sprintf function for string??
          sprintf((char*) cNameBuffer.c_str()       ,name.c_str(),uiK,uiK);
          sprintf((char*) cDescriptionBuffer.c_str(),desc.c_str(),uiK,uiK);

          parent.addOption(new Option<T>( cNameBuffer, (storage[uiK]), default_val, cDescriptionBuffer ));
        }

        return *this;
      }

      
      /**
       * Add option described by @name to the parent Options list,
       *   with @desc as an optional help description
       * instead of storing the value somewhere, a function of type
       * OptionFunc::Func is called.  It is upto this function to correctly
       * handle evaluating the option's value.
       */
      OptionSpecific&
      operator()(const std::string& name, OptionFunc::Func *func, const std::string& desc = "")
      {
        parent.addOption(new OptionFunc(name, parent, func, desc));
        return *this;
      }
    private:
      Options& parent;
    };
    
  }; /* namespace: program_options_lite */
}; /* namespace: df */
