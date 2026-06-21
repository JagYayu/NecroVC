local NecroVC = require("NecroVC.NecroVC")
local NecroVCAPI = require("NecroVC.API")

local ModEvent = require("necro.event.ModEvent")
local Settings = require("necro.config.Settings")

local assertNecroVCAPI = NecroVC.assertNecroVCAPI

local NecroVCSpeaker = {}

if NecroVCAPI and not NecroVCAPI.AudioPlayback.isOpened() then
	NecroVCAPI.AudioPlayback.open()
end

Settings.user.group({
	autoRegister = true,
	id = "speaker",
	name = "Speaker",
})

SettingVolume = Settings.user.percent({
	id = "speaker.volume",
	name = "Volume",
	default = 1,
	step = 0.05,
	smoothStep = 0.01,
	setter = function(value)
		if not NecroVCAPI then
			return
		end
		NecroVCAPI.AudioPlayback.setVolume(value)
	end,
})

function NecroVCSpeaker.addChannel(channelID)
	local api = assertNecroVCAPI(NecroVCAPI)

	if not api.AudioPlayback.hasChannel(channelID) then
		api.AudioPlayback.addChannel(channelID)
		return true
	else
		return false
	end
end

function NecroVCSpeaker.removeChannel(channelID)
	local api = assertNecroVCAPI(NecroVCAPI)

	if api.AudioPlayback.hasChannel(channelID) then
		api.AudioPlayback.removeChannel(channelID)
		return true
	else
		return false
	end
end

function NecroVCSpeaker.setChannelVolume(channelID, volume)
	local api = assertNecroVCAPI(NecroVCAPI)

	local channelAdded = NecroVCSpeaker.addChannel(channelID)
	api.AudioPlayback.setChannelVolume(channelID, volume)
	return channelAdded
end

function NecroVCSpeaker.open()
	local api = assertNecroVCAPI(NecroVCAPI)

	if api.AudioPlayback.isOpened() then
		return false
	end
	api.AudioPlayback.open()
	api.AudioPlayback.setVolume(SettingVolume)
	return true
end

function NecroVCSpeaker.close()
	local api = assertNecroVCAPI(NecroVCAPI)

	if not api.AudioPlayback.isOpened() then
		return false
	end
	api.AudioPlayback.close()
	return true
end

event.NecroVC_settingMicrophoneMonitor.add("speaker", "speaker", function(ev)
	local channelID = ev.playerID
	if not (NecroVCAPI and NecroVCAPI.AudioPlayback.isOpened()) then
		return
	end

	local volume = ev.value
	if volume > 0 then
		NecroVCSpeaker.setChannelVolume(channelID, volume)
	else
		NecroVCSpeaker.removeChannel(channelID)
	end
end)

if NecroVCAPI then
	ModEvent.addUnloadHandler(NecroVCSpeaker.close)
end

return NecroVCSpeaker
