R"luastring"--(

local MessageRole = {
	SYSTEM = 0,
	USER = 1,
	ASSISTANT = 2,
}

local Message = {
	role = MessageRole.SYSTEM,
	label = "",
	content = "",
	is_visible_to_textgen = true,
}

function Message:new(obj)
	obj = obj or {}
	setmetatable(obj, self)
	self.__index = self
	return obj
end

return {
	MessageRole = MessageRole,
	Message = Message,
}

-- DO NOT REMOVE THE NEXT LINE. It is used to load this file as a C++ string.
--)luastring"--"
