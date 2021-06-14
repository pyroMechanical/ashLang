#pragma once
#include "Value.h"
#include "Chunk.h"

namespace ash
{
	enum ObjType
	{
		OBJ_STRING,
		OBJ_FUNCTION,
		OBJ_EXTERNAL_FUNCTION,
		OBJ_STRUCT,
	};

	class Object
	{
	private:
		ObjType type;
	public:
		Object(ObjType type)
			:type(type) {}
		virtual ~Object() = default;

		virtual void print() = 0;
	};

	class StringObject : public Object
	{
	private:
		std::string string;
	public:
		StringObject(std::string chr_string)
			:Object(OBJ_STRING), string(chr_string) {}
	};

	struct FunctionArgument
	{
		ValueType valueType;
		ObjType objectType;

	};

	class FunctionObject : public Object
	{
	private:
		std::string name;
		std::vector<ValueType> args;
		Chunk chunk;
	public:
		FunctionObject()
			:Object(OBJ_FUNCTION), name("")
		{
			
		}
	};

	static inline bool isObjectType(Value value, ObjType type)
	{
		
	}
}