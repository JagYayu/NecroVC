local NecroVC = require("NecroVC.NecroVC")
local NecroVCAPI = require("NecroVC.API")

local ModEvent = require("necro.event.ModEvent")
local GameClient = require("necro.client.GameClient")
local Input = require("system.game.Input")
local Enum = require("system.utils.Enum")
local Utilities = require("system.utils.Utilities")
local CustomActions = require("necro.game.data.CustomActions")
local Settings = require("necro.config.Settings")
local LocalCoop = require("necro.client.LocalCoop")
local Controls = require("necro.config.Controls")
local Netplay = require("necro.network.Netplay")
local OrderedSelector = require("system.events.OrderedSelector")

local assert = assert

local NecroVCMicrophone = {}

NecroVCMicrophone.Option = Enum.sequence({
	AlwaysOff = 0,
	HoldToTalk = 1,
	SwitchToTalk = 2,
	VoiceActivated = 3,
	AlwaysOn = 4,
})
local NecroVCMicrophone_Option_AlwaysOff = NecroVCMicrophone.Option.AlwaysOff
local NecroVCMicrophone_Option_HoldToTalk = NecroVCMicrophone.Option.HoldToTalk
local NecroVCMicrophone_Option_SwitchToTalk = NecroVCMicrophone.Option.SwitchToTalk
local NecroVCMicrophone_Option_VoiceActivated = NecroVCMicrophone.Option.VoiceActivated
local NecroVCMicrophone_Option_AlwaysOn = NecroVCMicrophone.Option.AlwaysOn

NecroVCMicrophone.MaxCount = math.min( --
	Netplay.DEFAULT_MAX_LOCAL_COOP_PLAYERS,
	NecroVCAPI and NecroVCAPI.AudioCapture.getMaxCount() or math.huge
)

Settings.user.group({
	autoRegister = true,
	id = "microphone",
	name = "Microphones",
})

local microphoneEnableFunctions = Utilities.newTable(NecroVCMicrophone.MaxCount, 0)

MicrophonesData = Utilities.newTable(NecroVCMicrophone.MaxCount, 0)

function NecroVCMicrophone.getData(playerID)
	return MicrophonesData[playerID]
end

local settingMicrophoneMonitorSelectorFire = OrderedSelector.new(event.NecroVC_settingMicrophoneMonitor, {
	"speaker",
}).fire

local microphoneHotkeyArgs = {
	id = "microphoneHotkey",
	name = "Microphone hotkey",
	order = 15,
	keyBinding = "V",
	perPlayerBinding = true,
	enableIf = function(playerID)
		local microphoneData = MicrophonesData[playerID]
		return microphoneData.hotkeyDown == nil
	end,
	callback = function(playerID)
		local microphoneData = MicrophonesData[playerID]
		microphoneData.hotkeyActivated = not microphoneData.hotkeyActivated
	end,
}
NecroVCMicrophone.MicrophoneHotkey = CustomActions.registerHotkey(microphoneHotkeyArgs)

for playerID = 1, NecroVCMicrophone.MaxCount do
	local captureIndex = playerID - 1
	local playbackChannelID = playerID

	local microphoneData = MicrophonesData[playerID] or {
		HotkeyActivated = nil,
		hotkeyDown = nil,
	}
	MicrophonesData[playerID] = microphoneData

	local groupID = "microphone." .. playerID

	Settings.user.group({
		autoRegister = true,
		id = groupID,
		name = "Player " .. playerID,
		order = playerID,
	})

	local setting = "SettingMicrophone" .. playerID

	local gOption = setting .. "Option"
	_G[gOption] = Settings.user.enum({
		id = groupID .. ".option",
		name = "Option",
		order = 1,
		default = NecroVCMicrophone_Option_HoldToTalk,
		enum = NecroVCMicrophone.Option,
		setter = function(value)
			if not NecroVCAPI then
				return
			end
			if value == NecroVCMicrophone_Option_AlwaysOff then
				if NecroVCAPI.AudioCapture.isOpened(captureIndex) then
					NecroVCAPI.AudioCapture.close(captureIndex)
				end
			else
				if not NecroVCAPI.AudioCapture.isOpened(captureIndex) then
					NecroVCAPI.AudioCapture.open(captureIndex)
				end
			end
			if value == NecroVCMicrophone_Option_HoldToTalk then
				microphoneData.hotkeyActivated = nil
				microphoneData.hotkeyDown = false
			elseif value == NecroVCMicrophone_Option_SwitchToTalk then
				microphoneData.hotkeyActivated = false
				microphoneData.hotkeyDown = nil
			else
				microphoneData.hotkeyActivated = nil
				microphoneData.hotkeyDown = nil
			end
		end,
	})

	local gVolume = setting .. "Volume"
	_G[gVolume] = Settings.user.percent({
		id = groupID .. ".volume",
		name = "Volume",
		order = 2,
		default = 1,
		maximum = 3,
		sliderMaximum = 2,
		step = 0.05,
		smoothStep = 0.01,
		setter = function(value)
			if not NecroVCAPI or not NecroVCAPI.AudioCapture.isOpened(captureIndex) then
				return
			end
			NecroVCAPI.AudioCapture.setVolume(captureIndex, value)
		end,
	})

	local gMonitor = setting .. "Monitor"
	_G[gMonitor] = Settings.user.percent({
		id = groupID .. ".monitor",
		name = "Self monitor",
		order = 3,
		default = 0,
		maximum = 2,
		sliderMaximum = 1,
		step = 0.05,
		smoothStep = 0.01,
		setter = function(value)
			settingMicrophoneMonitorSelectorFire({
				playerID = playerID,
				value = value,
			})
		end,
	})

	microphoneEnableFunctions[playerID] = function()
		assert(NecroVCAPI)
		NecroVCAPI.AudioCapture.setVolume(captureIndex, _G[gVolume])
	end

	event.tick.add("handleMicrophoneHotkey" .. playerID, "customHotkeys", function()
		if microphoneData.hotkeyDown == nil then
			return
		end

		microphoneData.hotkeyDown = false
		for _, key in
			ipairs(
				Controls.getActionKeyBinds(NecroVCMicrophone.MicrophoneHotkey, LocalCoop.getControllerID(playerID))
					or NecroVC.emptyTable
			)
		do
			if Input.keyDown(key) then
				microphoneData.hotkeyDown = true
				break
			end
		end
	end)

	local function checkVoiceInputActivity()
		local option = _G[gOption]
		if option == NecroVCMicrophone_Option_HoldToTalk then
			if not microphoneData.hotkeyDown then
				return false
			end
		elseif option == NecroVCMicrophone_Option_SwitchToTalk then
			if not microphoneData.hotkeyActivated then
				return false
			end
		elseif option == NecroVCMicrophone_Option_VoiceActivated then
			-- TODO
		elseif option ~= NecroVCMicrophone_Option_AlwaysOn then
			return false
		end
		return true
	end

	local function handleVoiceData(data)
		assert(NecroVCAPI)

		if _G[gMonitor] > 0 then
			NecroVCAPI.AudioPlayback.push(playbackChannelID, data)
		end

		if GameClient.isLoggedIn() then
			GameClient.sendReliable(NecroVC.Netplay_MessageType_VoiceData, { data, 1 }, Netplay.Channel.CHAT)
		end
	end

	event.tick.add("handleVoiceInput" .. playerID, "input", function()
		if not NecroVCAPI or not checkVoiceInputActivity() then
			return
		end

		for _ = 1, 100 do
			local data, errMsg = NecroVCAPI.AudioCapture.pull(captureIndex)
			if not data then
				log.error(errMsg)
			elseif data ~= "" then
				handleVoiceData(data)
			else
				break
			end
		end
	end)
end

function NecroVCMicrophone.enable()
	if not NecroVCAPI then
		return
	end
	for playerID = 1, NecroVCMicrophone.MaxCount do
		microphoneEnableFunctions[playerID]()
	end
end

function NecroVCMicrophone.disable()
	if not NecroVCAPI then
		return
	end
	local count = 0
	for playerID = 1, NecroVCMicrophone.MaxCount do
		local captureIndex = playerID - 1
		if NecroVCAPI.AudioCapture.isOpened(captureIndex) then
			NecroVCAPI.AudioCapture.close(captureIndex)
			count = count + 1
		end
	end
	return count
end

ModEvent.addUnloadHandler(NecroVCMicrophone.disable)

-- event.tick.override("NecroVC_action_microphoneHotkey", function(func, ...)
-- 	local args = microphoneHotkeyArgs
-- 	local id = NecroVCMicrophone.MicrophoneHotkey

-- 	for _, playerID in ipairs(LocalCoop.getLocalPlayerIDs()) do
-- 		if
-- 			args.enableIf(playerID)
-- 			and Controls.actionKey(id, LocalCoop.getControllerID(playerID))
-- 			and args.callback(playerID) ~= false
-- 		then
-- 			Controls.consumeActionKey(id, LocalCoop.getControllerID(playerID))
-- 		end
-- 	end
-- end)

return NecroVCMicrophone
