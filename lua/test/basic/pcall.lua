function okfunc() 
    return "A_OK"
end

function errfunc ()
    error("this throws an error")
end

local status, result = pcall(okfunc)
print("status: " .. tostring(status) .. ", result: " .. result)

status, result = pcall(errfunc)
print("status: " .. tostring(status) .. ", message: " .. result)