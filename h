--[[
    🕷️ TARANTULA // STRICT ENVIRONMENT INTEGRITY & ACTIVITY MONITOR
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

-- 3. Persistent HWID Resolution
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
local function reportTamperAndHalt(reason)
    if isBlacklisted then return end
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
-- 5. PRE-EXECUTION INTEGRITY AUDIT
-- ====================================================================

-- Check 1: Audit Function Integrity (Checks for wrappers/hooks on core functions)
local sensitiveFunctions = {
    print = print,
    warn = warn,
    loadstring = loadstring,
    httpRequest = httpRequest
}

for name, fn in pairs(sensitiveFunctions) do
    if islclosure and islclosure(fn) then
        reportTamperAndHalt("Hooked_" .. name .. "_islclosure")
        return
    end
    if debug and debug.getinfo then
        local info = debug.getinfo(fn)
        if info and info.what ~= "C" then
            reportTamperAndHalt("Hooked_" .. name .. "_non_C")
            return
        end
    end
end

-- Check 2: Audit Global Pollution (Detects extra variables declared before execution)
local cleanGlobals = {
    ["loader_key"] = true,
    ["_TARANTULA_TOKEN"] = true,
    ["_TARANTULA_VALIDATED"] = true
}

local function auditEnvironment(env, envName)
    if not env then return true end
    for key, _ in pairs(env) do
        local keyStr = tostring(key)
        -- Flag any non-standard global injections
        if keyStr:sub(1, 1) ~= "_" and not cleanGlobals[keyStr] then
            if keyStr:lower():find("spy") or keyStr:lower():find("dump") or keyStr:lower():find("hook") or keyStr:lower():find("test") then
                reportTamperAndHalt("Polluted_" .. envName .. "_" .. keyStr)
                return false
            end
        end
    end
    return true
end

if not auditEnvironment(_G, "G") then return end
if getgenv and not auditEnvironment(getgenv(), "GENV") then return end

-- Check 3: Console Activity Audit via LogService
pcall(function()
    local LogService = game:GetService("LogService")
    if LogService and LogService.GetLogHistory then
        local logs = LogService:GetLogHistory()
        for i = #logs, math.max(1, #logs - 5), -1 do
            local msg = tostring(logs[i].message or "")
            -- Detect third-party logging or debugging signatures in recent console output
            if msg:find("SimpleSpy") or msg:find("HttpSpy") or msg:find("Hooked") or msg:find("Dump") then
                reportTamperAndHalt("Suspicious_pre_execution_log")
                return
            end
        end
    end
end)

if isBlacklisted then return end

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
-- 7. EXECUTE SEALED PAYLOAD WITH POST-EXECUTION WATCHDOG
-- ====================================================================
local token = response.Headers and (response.Headers["x-tarantula-token"] or response.Headers["X-Tarantula-Token"])
if not token then
    reportTamperAndHalt("Missing_server_token")
    return
end

if getgenv then getgenv()._TARANTULA_TOKEN = token end

local executePayload, compileErr = loadstring(response.Body)
if not executePayload then
    reportTamperAndHalt("Invalid_payload_structure")
    return
end

-- Run payload safely
local execOk, execErr = pcall(executePayload)

-- Verification: confirm genuine payload execution completed
if not getgenv or getgenv()._TARANTULA_VALIDATED ~= token then
    reportTamperAndHalt("Unauthorized_payload_executed")
    return
end

-- Clean memory
if getgenv then
    getgenv()._TARANTULA_TOKEN = nil
    getgenv()._TARANTULA_VALIDATED = nil
end

-- ====================================================================
-- 8. POST-EXECUTION MONITORING WATCHDOG
-- Runs continually to detect post-execution tampering, hooks, or dumping
-- ====================================================================
task.spawn(function()
    while task.wait(5) do
        -- 1. Check if print, loadstring, or network functions were hooked after running
        for name, fn in pairs(sensitiveFunctions) do
            if (islclosure and islclosure(fn)) or (debug and debug.getinfo and debug.getinfo(fn).what ~= "C") then
                reportTamperAndHalt("Post_execution_hook_" .. name)
                break
            end
        end

        -- 2. Detect subsequent spy injections
        local g = (getgenv and getgenv()) or _G
        if g.SimpleSpyExecuted or g.HttpSpy or g.Spy or g.Dumper or g.DumpString then
            reportTamperAndHalt("Post_execution_spy_detected")
            break
        end

        if isBlacklisted then break end
    end
end)
