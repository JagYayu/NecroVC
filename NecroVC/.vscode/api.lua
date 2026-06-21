--- @meta

--- @class NecroVC.API
local api = {
	AudioCapture = {},
	AudioPlayback = {},
	Context = {},
}

--- @return boolean
--- @nodiscard
function api.isOpened() end

function api.open() end

function api.close() end

--- @return string data
--- @nodiscard
function api.pull() end

--- @param data string
function api.push(data) end

--- @return 4 | integer index
--- @nodiscard
function api.AudioCapture.getMaxCount() end

--- @param index integer
--- @return boolean success
function api.AudioCapture.isOpened(index) end

--- @param index integer
function api.AudioCapture.open(index) end

--- @param index integer
function api.AudioCapture.close(index) end

--- @param index integer
--- @return string data
--- @nodiscard
function api.AudioCapture.pull(index) end

--- @param index integer
--- @return string? data0
--- @return string? data1
--- @return string? data2
--- @return string? data3
--- @nodiscard
function api.AudioCapture.pullAll(index) end

--- @return boolean
--- @nodiscard
function api.AudioPlayback.isOpened() end

function api.AudioPlayback.open() end

function api.AudioPlayback.close() end

--- @return number volume
--- @nodiscard
function api.AudioPlayback.getVolume() end

--- @param volume number
function api.AudioPlayback.setVolume(volume) end

--- @param id integer
--- @return number volume
--- @nodiscard
function api.AudioPlayback.getChannelVolume(id) end

--- @param id integer
--- @param volume number
function api.AudioPlayback.setChannelVolume(id, volume) end

--- @param channelID integer
--- @param data string
function api.AudioPlayback.push(channelID, data) end

--- @return boolean inited
--- @nodiscard
function api.Context.isInited() end

function api.Context.init() end

function api.Context.deinit() end

function api.Context.listCaptureDevices() end

function api.Context.listPlaybackDevices() end

return api
