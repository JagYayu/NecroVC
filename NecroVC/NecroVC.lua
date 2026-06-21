local Netplay = require("necro.network.Netplay")

local NecroVC = {}

NecroVC.Netplay_MessageType_VoiceData = Netplay.MessageType.extend("NecroVC_VoiceData")

--- @param api false | NecroVC.API
--- @return NecroVC.API
function NecroVC.assertNecroVCAPI(api)
	assert(api, "VCLib api not available")
	return api
end

function NecroVC.emptyFunction() end

NecroVC.emptyTable = setmetatable({}, { __newindex = NecroVC.emptyFunction })

return NecroVC
