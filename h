--[[
    🕷️ TARANTULA // STRICT PRINT & EXECUTION SURVEILLANCE
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
-- 4. AUTOMATED BLACKLIST REPORTER
-- ====================================================================
local isBlacklisted = false
local isVerified = false

local function reportTamperAndHalt(reason)
    if isBlacklisted or isVerified then return end
    isBlacklisted = true

    local cleanReason = tostring(reason):gsub("%s+", "_"):gsub("[^%w_%-]", "")
    
    pcall(function()
        httpRequest({
            Url = SERVER_URL .. "/report_tamper?key=" .. tostring(LICENSE_KEY) 
                .. "&hwid=" .. tostring(clientHWID) 
                .. "&rbx=" .. tostring(rbxUsername) 
                .. "&reason=" .. tostring(cleanReason),
            Method = "GET",
            Headers = { ["x-tarantula-auth"] = AUTH_HEADER }
        })
    end)
    
    print("[tarantula] what u tryna do bud")
end

-- ====================================================================
-- 5. REAL-TIME PRINT MONITOR (Pre-Verification)
-- ====================================================================
local LogService = game:GetService("LogService")

local function isAuthorizedPrint(msg)
    if isVerified then return true end
    -- Whitelist prints
    if msg:find("%[tarantula%]") or msg:find("%[Whitelist%]") or msg:find("fabian") then
        return true
    end
    -- System/Roblox internal prints
    if msg:find("The Current Identity") or msg:find("Roblox Version") or msg:find("Replication") or msg:find("HttpTrace") or msg:find("CoreGui") then
        return true
    end
    return false
end

-- Monitor any new print while authenticating
local printConnection
printConnection = LogService.MessageOut:Connect(function(message, messageType)
    if isVerified then return end
    local msgStr = tostring(message or "")
    if not isAuthorizedPrint(msgStr) then
        reportTamperAndHalt("PrePrint_" .. msgStr:sub(1, 25))
    end
end)

-- Check function integrity
local sensitiveFunctions = {
    print = print,
    warn = warn,
    loadstring = loadstring,
    httpRequest = httpRequest
}

for name, fn in pairs(sensitiveFunctions) do
    if (islclosure and islclosure(fn)) or (debug and debug.getinfo and debug.getinfo(fn).what ~= "C") then
        reportTamperAndHalt("Hooked_" .. name)
        return
    end
end

-- ====================================================================
-- 6. WHITELIST VERIFICATION GATE
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
-- 7. EXECUTE SEALED PAYLOAD
-- ====================================================================
local token = response.Headers and (response.Headers["x-tarantula-token"] or response.Headers["X-Tarantula-Token"])
if not token then
    reportTamperAndHalt("Missing_server_token")
    return
end

-- Unlock: payload is verified, disconnect print monitor
isVerified = true
if printConnection and printConnection.Disconnect then
    pcall(function() printConnection:Disconnect() end)
end

if getgenv then getgenv()._TARANTULA_TOKEN = token end

local executePayload, compileErr = loadstring(response.Body)
if not executePayload then
    reportTamperAndHalt("Invalid_payload_structure")
    return
end

local execOk, execErr = pcall(executePayload)

if not getgenv or getgenv()._TARANTULA_VALIDATED ~= token then
    reportTamperAndHalt("Unauthorized_or_fake_payload_executed")
    return
end

if getgenv then
    getgenv()._TARANTULA_TOKEN = nil
    getgenv()._TARANTULA_VALIDATED = nil
end
