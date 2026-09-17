--[[
    🕷️ TARANTULA // PROTECTED LOADER + ANTI-TAMPER
    Hosted at: https://raw.githubusercontent.com/reaper2lmao-lang/test/refs/heads/main/h
]]

-- 1. Configuration & Key Grabber
local SERVER_URL  = "https://winter-limit-acb5.breathness69.workers.dev"
local AUTH_HEADER = "TARANTULA-EX-v1"

local rawKey = loader_key 
    or (getgenv and getgenv().loader_key) 
    or _G.loader_key

if not rawKey then
    print("[tarantula] invalid key")
    return
end

local LICENSE_KEY = tostring(rawKey):gsub("^%s*(.-)%s*$", "%1")
if #LICENSE_KEY == 0 then
    print("[tarantula] invalid key")
    return
end

-- 2. Universal HTTP Resolver
local httpRequest = (syn and syn.request) 
    or request 
    or http_request 
    or (http and http.request)

if not httpRequest then
    warn("[tarantula] Executor does not support HTTP requests.")
    return
end

-- 3. Persistent HWID Detection
local function getClientHWID()
    local hwidFile = "tarantula_hwid.dat"
    if isfile and readfile and isfile(hwidFile) then
        local saved = readfile(hwidFile)
        if saved and #saved > 5 then return saved end
    end

    local finalHWID = nil
    if gethwid then 
        finalHWID = tostring(gethwid())
    elseif syn and syn.get_hwid then 
        finalHWID = tostring(syn.get_hwid())
    end

    if not finalHWID or #finalHWID == 0 then
        local ok, cid = pcall(function() return game:GetService("RbxAnalyticsService"):GetClientId() end)
        if ok and cid and #cid > 5 then finalHWID = tostring(cid) end
    end

    if not finalHWID or #finalHWID == 0 then
        local ok, uid = pcall(function() return tostring(game:GetService("Players").LocalPlayer.UserId) end)
        if ok and uid then finalHWID = "ROBLOX_UID_" .. uid end
    end

    if not finalHWID then
        finalHWID = "FALLBACK_" .. tostring(math.random(100000, 999999))
    end

    if writefile then pcall(function() writefile(hwidFile, finalHWID) end) end
    return finalHWID
end

local clientHWID = getClientHWID()
local rbxUsername = "Unknown"
pcall(function() rbxUsername = game:GetService("Players").LocalPlayer.Name end)

-- ====================================================================
-- 4. ANTI-TAMPER / CRACK DETECTIONS
-- ====================================================================
local function reportTamperAndHalt(reason)
    pcall(function()
        httpRequest({
            Url = SERVER_URL .. "/report_tamper?key=" .. tostring(LICENSE_KEY) 
                .. "&hwid=" .. tostring(clientHWID) 
                .. "&rbx=" .. tostring(rbxUsername) 
                .. "&reason=" .. tostring(reason),
            Method = "GET",
            Headers = { ["x-tarantula-auth"] = AUTH_HEADER }
        })
    end)
    print("[tarantula] what u tryna do bud")
end

-- Detection 1: loadstring hooked by Lua closure
local function checkLoadstringIntegrity()
    if islclosure and islclosure(loadstring) then return false end
    if debug and debug.getinfo then
        local info = debug.getinfo(loadstring)
        if info and info.what ~= "C" then return false end
    end
    return true
end

-- Detection 2: httpRequest / request hooked
local function checkRequestIntegrity()
    if islclosure and islclosure(httpRequest) then return false end
    if debug and debug.getinfo then
        local info = debug.getinfo(httpRequest)
        if info and info.what ~= "C" then return false end
    end
    return true
end

-- Detection 3: Known HTTP spies / dumping scripts in environment
local function checkKnownTools()
    local g = (getgenv and getgenv()) or _G
    if g.SimpleSpyExecuted or g.HttpSpy or g.Spy or g.Dumper or g.DumpString then
        return false
    end
    return true
end

-- Run detections
if not checkLoadstringIntegrity() then
    reportTamperAndHalt("Hooked loadstring")
    return
end

if not checkRequestIntegrity() then
    reportTamperAndHalt("Hooked httpRequest")
    return
end

if not checkKnownTools() then
    reportTamperAndHalt("Known spy/dumping tool active")
    return
end

-- ====================================================================
-- 5. WHITELIST VERIFICATION GATE
-- ====================================================================
print("[tarantula] Verifying license...")

local queryUrl = SERVER_URL 
    .. "/verify?key=" .. tostring(LICENSE_KEY) 
    .. "&hwid=" .. tostring(clientHWID) 
    .. "&rbx=" .. tostring(rbxUsername)

local response = httpRequest({
    Url = queryUrl,
    Method = "GET",
    Headers = { ["x-tarantula-auth"] = AUTH_HEADER }
})

if not response then
    warn("[tarantula] Failed to reach verification server.")
    return
end

local body = tostring(response.Body):gsub("^%s*(.-)%s*$", "%1")

if response.StatusCode == 401 or body == "invalid key" then
    print("[tarantula] invalid key")
    return
elseif response.StatusCode ~= 200 or body == "what u tryna do bud" then
    print("[tarantula] what u tryna do bud")
    return
end

-- ====================================================================
-- 6. RUN PROTECTED SCRIPT PAYLOAD
-- ====================================================================
local executePayload, compileErr = loadstring(response.Body)
if not executePayload then
    warn("[tarantula] Failed to compile payload: " .. tostring(compileErr))
    return
end

executePayload()
