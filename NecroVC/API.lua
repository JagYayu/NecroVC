local Utilities = require("system.utils.Utilities")

local presentStandardLibrary, StandardLibrary = pcall(require, "core.StandardLibrary")
if not presentStandardLibrary then
	return false
end

local loadlib = StandardLibrary.package.loadlib
local path = "NecroVC/NecroVC.dll"

local function fetch(fname)
	local func = loadlib(path, fname)
	if not func then
		error(('C function "%s" not found'):format(fname))
	end
	return func
end

--- @type NecroVC.API
local NecroVCAPI = {}

NecroVCAPI.isOpened = fetch("NecroVC_LIsOpened")
NecroVCAPI.open = fetch("NecroVC_LOpen")
NecroVCAPI.close = fetch("NecroVC_LClose")
NecroVCAPI.pull = fetch("NecroVC_LPull")
NecroVCAPI.push = fetch("NecroVC_LPush")

NecroVCAPI.AudioCapture = {
	getMaxCount = fetch("NecroVC_AudioCapture_LGetMaxCount"),
	isOpened = fetch("NecroVC_AudioCapture_LIsOpened"),
	open = fetch("NecroVC_AudioCapture_LOpen"),
	close = fetch("NecroVC_AudioCapture_LClose"),
	getVolume = fetch("NecroVC_AudioCapture_LGetVolume"),
	setVolume = fetch("NecroVC_AudioCapture_LSetVolume"),
	pull = fetch("NecroVC_AudioCapture_LPull"),
	pullAll = fetch("NecroVC_AudioCapture_LPullAll"),
}
NecroVCAPI.AudioCapture = Utilities.readOnlyTable(NecroVCAPI.AudioCapture)

NecroVCAPI.AudioPlayback = {
	isOpened = fetch("NecroVC_AudioPlayback_LIsOpened"),
	open = fetch("NecroVC_AudioPlayback_LOpen"),
	close = fetch("NecroVC_AudioPlayback_LClose"),
	getVolume = fetch("NecroVC_AudioPlayback_LGetVolume"),
	setVolume = fetch("NecroVC_AudioPlayback_LSetVolume"),
	hasChannel = fetch("NecroVC_AudioPlayback_LHasChannel"),
	addChannel = fetch("NecroVC_AudioPlayback_LAddChannel"),
	removeChannel = fetch("NecroVC_AudioPlayback_LRemoveChannel"),
	getChannelVolume = fetch("NecroVC_AudioPlayback_LGetChannelVolume"),
	setChannelVolume = fetch("NecroVC_AudioPlayback_LSetChannelVolume"),
	push = fetch("NecroVC_AudioPlayback_LPush"),
}
NecroVCAPI.AudioPlayback = Utilities.readOnlyTable(NecroVCAPI.AudioPlayback)

NecroVCAPI.Context = {
	isInited = fetch("NecroVC_Context_LIsInited"),
	init = fetch("NecroVC_Context_LInit"),
	deinit = fetch("NecroVC_Context_LDeinit"),
	listCaptureDevices = fetch("NecroVC_Context_LListCaptureDevices"),
	listPlaybackDevices = fetch("NecroVC_Context_LListPlaybackDevices"),
}
NecroVCAPI.Context = Utilities.readOnlyTable(NecroVCAPI.Context)

return NecroVCAPI
