--[[
    🕷️ TARANTULA // GITHUB PAYLOAD SCRIPT
    Hosted at: https://raw.githubusercontent.com/reaper2lmao-lang/test/refs/heads/main/h
]]

-- 1. Configuration & Key Grabber
local SERVER_URL  = "https://winter-limit-acb5.breathness69.workers.dev/verify"
local AUTH_HEADER = "TARANTULA-EX-v1"

-- Grab and trim key
local rawKey = loader_key 
    or (getgenv and getgenv().loader_key) 
    or _G.loader_key

if not rawKey then
    print("[tarantula] invalid key")
    return
end

-- Auto-trim any accidental spaces or tabs
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
        local savedHWID = readfile(hwidFile)
        if savedHWID and #savedHWID > 5 then
            return savedHWID
        end
    end

    local finalHWID = nil
    if gethwid then 
        finalHWID = tostring(gethwid())
    elseif syn and syn.get_hwid then 
        finalHWID = tostring(syn.get_hwid())
    end

    if not finalHWID or #finalHWID == 0 then
        local ok, clientId = pcall(function()
            return game:GetService("RbxAnalyticsService"):GetClientId()
        end)
        if ok and clientId and #clientId > 5 then
            finalHWID = tostring(clientId)
        end
    end

    if not finalHWID or #finalHWID == 0 then
        local ok, uid = pcall(function()
            return tostring(game:GetService("Players").LocalPlayer.UserId)
        end)
        if ok and uid then
            finalHWID = "ROBLOX_UID_" .. uid
        end
    end

    if not finalHWID then
        finalHWID = "FALLBACK_DEVICE_" .. tostring(math.random(100000, 999999))
    end

    if writefile then
        pcall(function() writefile(hwidFile, finalHWID) end)
    end

    return finalHWID
end

local clientHWID = getClientHWID()

local rbxUsername = "Unknown"
pcall(function()
    rbxUsername = game:GetService("Players").LocalPlayer.Name
end)

-- 4. Whitelist Verification Gate
print("[tarantula] Verifying license...")

local queryUrl = SERVER_URL 
    .. "?key=" .. tostring(LICENSE_KEY) 
    .. "&hwid=" .. tostring(clientHWID) 
    .. "&rbx=" .. tostring(rbxUsername)

local response = httpRequest({
    Url = queryUrl,
    Method = "GET",
    Headers = {
        ["x-tarantula-auth"] = AUTH_HEADER
    }
})

if not response then
    warn("[tarantula] Failed to reach verification server.")
    return
end

local body = tostring(response.Body):gsub("^%s*(.-)%s*$", "%1")

-- Rejection Gates
if response.StatusCode == 401 or body == "invalid key" then
    print("[tarantula] invalid key")
    return
elseif response.StatusCode ~= 200 or body == "what u tryna do bud" then
    print("[tarantula] what u tryna do bud")
    return
end

-- =========================================================
-- 5. RUN PROTECTED SCRIPT PAYLOAD (From Cloudflare Worker)
-- =========================================================
local executePayload, compileErr = loadstring(response.Body)
if not executePayload then
    warn("[tarantula] Failed to compile payload: " .. tostring(compileErr))
    return
end

executePayload()
