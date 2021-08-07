#pragma once

namespace ash
{
	enum AllocationType
	{
		STRING_ALLOC,
		FUNCTION_ALLOC,
		CLOSURE_ALLOC,
		TYPE_ALLOC,

	};

	struct Allocation
	{
		AllocationType type;
	};
}