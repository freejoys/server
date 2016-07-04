local skynet = require "skynet"
require "skynet.manager"

local function main()
end

skynet.start(function()
	local success, ret = pcall(main)
	if success == false then
		skynet.abort()
	end
	skynet.exit()
end)
