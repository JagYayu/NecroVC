local StandardLibrary = require("core.StandardLibrary")
local Netplay = require("necro.network.Netplay")
local ModEvent = require("necro.event.ModEvent")
local Settings = require("necro.config.Settings")
local Enum = require("system.utils.Enum")
local CustomActions = require("necro.game.data.CustomActions")
local Controls = require("necro.config.Controls")
local Input = require("system.game.Input")
local GameClient = require("necro.client.GameClient")
local ServerSocket = require("necro.server.ServerSocket")
local ServerPlayerList = require("necro.server.ServerPlayerList")

local NecroVC = {}

local vcIsOpened = StandardLibrary.package.loadlib("NecroVC/NecroVC.dll", "NecroVC_IsOpened")
local vcOpen = StandardLibrary.package.loadlib("NecroVC/NecroVC.dll", "NecroVC_Open")
local vcClose = StandardLibrary.package.loadlib("NecroVC/NecroVC.dll", "NecroVC_Close")
local vcPull = StandardLibrary.package.loadlib("NecroVC/NecroVC.dll", "NecroVC_Pull")
local vcPush = StandardLibrary.package.loadlib("NecroVC/NecroVC.dll", "NecroVC_Push")

NecroVC.vcIsOpened = vcIsOpened
NecroVC.vcOpen = vcOpen
NecroVC.vcClose = vcClose
NecroVC.vcPull = vcPull
NecroVC.vcPush = vcPush

NecroVC.Netplay_MessageType_VoiceData = Netplay.MessageType.extend("NecroVC_VoiceData")

function NecroVC.pushSpatial()
	error("not implemented yet")
end

ModEvent.addUnloadHandler(vcClose)

SettingEnable = Settings.user.bool({
	order = 0,
	id = "enable",
	name = "VCLib",
	default = true,
	setter = function(value)
		if value then
			vcOpen()
		else
			vcClose()
		end
	end,
})

Settings.user.action({
	autoRegister = true,
	order = 1,
	id = "reload",
	name = "VCLib reload",
	action = function()
		if vcIsOpened() then
			vcClose()
		end
		if SettingEnable then
			vcOpen()
		end
	end,
})

NecroVC.MicrophoneOption = Enum.sequence({
	AlwaysOff = 0,
	HoldToTalk = 1,
	SwitchToTalk = 2,
	VoiceActivated = 3,
	AlwaysOn = 4,
})

local microphoneHotkeyActivated
local microphoneHotkeyDown

SettingMicrophoneOption = Settings.user.enum({
	id = "microphoneOption",
	name = "Microphone option",
	order = 11,
	default = NecroVC.MicrophoneOption.HoldToTalk,
	enum = NecroVC.MicrophoneOption,
	setter = function(value)
		if value == NecroVC.MicrophoneOption.HoldToTalk then
			microphoneHotkeyActivated = nil
			microphoneHotkeyDown = false
		elseif value == NecroVC.MicrophoneOption.SwitchToTalk then
			microphoneHotkeyActivated = false
			microphoneHotkeyDown = nil
		else
			microphoneHotkeyActivated = nil
			microphoneHotkeyDown = nil
		end
	end,
})

NecroVC.HotKey_MicrophoneHotkey = CustomActions.registerHotkey({
	id = "microphoneHotkey",
	name = "Microphone Hotkey",
	order = 12,
	keyBinding = "V",
	callback = function()
		microphoneHotkeyActivated = not microphoneHotkeyActivated
	end,
})

function NecroVC.isMicrophoneHotkeyActivated()
	return microphoneHotkeyActivated
end

function NecroVC.isMicrophoneHotkeyDown()
	return microphoneHotkeyDown
end

event.tick.override("NecroVC_action_microphoneHotkey", function(func, ...)
	if microphoneHotkeyDown == nil then
		func(...)
	end
end)

event.tick.add("handleMicrophoneHotkey" .. NecroVC.HotKey_MicrophoneHotkey, "customHotkeys", function()
	if microphoneHotkeyDown ~= nil then
		microphoneHotkeyDown = false
		for _, key in ipairs(Controls.getMiscKeyBinds(NecroVC.HotKey_MicrophoneHotkey) or {}) do
			if Input.keyDown(key) then
				microphoneHotkeyDown = true
				break
			end
		end
	end
end)

SettingMonitorMicrophone = Settings.user.percent({
	id = "monitorMicrophone",
	name = "Monitor microphone",
	default = 0,
	maximum = 10,
	sliderMaximum = 1,
	step = 0.05,
	smoothStep = 0.01,
})

local function checkVoiceInputActivity()
	local option = SettingMicrophoneOption
	if option == NecroVC.MicrophoneOption.HoldToTalk then
		if not microphoneHotkeyDown then
			return false
		end
	elseif option == NecroVC.MicrophoneOption.SwitchToTalk then
		if not microphoneHotkeyActivated then
			return false
		end
	elseif option == NecroVC.MicrophoneOption.VoiceActivated then
		-- TODO
	elseif option ~= NecroVC.MicrophoneOption.AlwaysOn then
		return false
	end
	return true
end

local VoiceDataMessage_data = 1
local VoiceDataMessage_volume = 2
local VoiceDataMessage_position = 3
local cachedVoiceDataMessage = { false, false, false }

local function packVoiceDataMessage(data, volume, position)
	cachedVoiceDataMessage[VoiceDataMessage_data] = data
	cachedVoiceDataMessage[VoiceDataMessage_volume] = volume
	cachedVoiceDataMessage[VoiceDataMessage_position] = position or false
	-- cachedVoiceDataMessage[4] = NetClock.getTime()
	return cachedVoiceDataMessage
end

local function handleVoiceData(data)
	if SettingMonitorMicrophone > 0 then
		vcPush(data) -- self monitor
		-- TODO allow adjust volume
	end

	GameClient.sendReliable(NecroVC.Netplay_MessageType_VoiceData, packVoiceDataMessage(data, 1), Netplay.Channel.CHAT)
end

event.tick.add("handleVoiceInputs", "input", function()
	if checkVoiceInputActivity() then
		for _ = 1, 100 do
			local data, errMsg = vcPull()
			if not data then
				log.error(errMsg)
			elseif data ~= "" then
				handleVoiceData(data)
			else
				break
			end
		end
	end
end)

event.serverMessage.add("broadcastVoiceData", NecroVC.Netplay_MessageType_VoiceData, function(ev)
	local responseTargets = {}
	for _, playerID in ipairs(ServerPlayerList.allPlayers()) do
		if playerID ~= ev.playerID then
			responseTargets[#responseTargets + 1] = playerID
		end
	end
	if responseTargets[1] then
		-- ev.message[0] = ev.playerID
		ServerSocket.sendReliable(
			NecroVC.Netplay_MessageType_VoiceData,
			ev.message,
			responseTargets,
			Netplay.Channel.CHAT
		)
	end
end)

event.clientMessage.add("receiveVoiceData", NecroVC.Netplay_MessageType_VoiceData, function(message)
	vcPull(message[VoiceDataMessage_data])
end)

microphoneHotkeyActivated, microphoneHotkeyDown = script.persist(function()
	return microphoneHotkeyActivated, microphoneHotkeyDown
end)

return NecroVC
