#ifndef VARIABLES_H
#define VARIABLES_H

#include "consoleitem.h"
#include "util/text.h"
#include <boost/function.hpp>
#include <string>
#include <map>

class Variable : public ConsoleItem
{
public:	
	Variable(std::string const& name) : m_name(name) {}
	Variable() {}	
	virtual ~Variable() {}

	std::string const& getName() { return m_name; }
	virtual void reset() {}
	
protected:
	std::string m_name;
};

template<class T>
class TVariable : public Variable
{
public:
	
	typedef boost::function<void (T const&)> CallbackT;
	
	TVariable(std::string name, T* src, T defaultValue, CallbackT const& callback = CallbackT() )
	: Variable(name), m_src(src), m_defaultValue(defaultValue), m_callback(callback)
	{ reset(); }
	
	TVariable()
	: Variable(), m_src(NULL), m_defaultValue(T()), m_callback(NULL) {}
	
	void reset() { *m_src = m_defaultValue; }
	
	std::string invoke(std::list<std::string> const& args)
	{
		if (!args.empty())
		{
			T oldValue = *m_src;
			*m_src = convert<T>::value(*args.begin());
			if ( m_callback ) m_callback(oldValue);
	
			return std::string();
		}else
		{
			//return m_name + " IS \"" + cast<std::string>(*m_src) + '"';
			return convert<std::string>::value(*m_src);
		}
	}
	
protected:
	T* m_src;
	T m_defaultValue;
	CallbackT m_callback;
};

typedef TVariable<int> IntVariable;
typedef TVariable<float> FloatVariable;
typedef TVariable<std::string> StringVariable;

class EnumVariable : public IntVariable
{
public:
	typedef std::map<std::string, int, IStrCompare> MapType;
	typedef std::map<int, std::string> ReverseMapType;
		
	EnumVariable(std::string name, int* src, int defaultValue, MapType const& mapping, CallbackT const& func = CallbackT());
	EnumVariable();
	virtual ~EnumVariable();
	
	std::string invoke(const std::list<std::string> &args);
	
	virtual std::string completeArgument(int idx, std::string const& beginning);
	
protected:
	ReverseMapType m_reverseMapping;
	MapType m_mapping;
};

#endif  // _VARIABLES_H_
