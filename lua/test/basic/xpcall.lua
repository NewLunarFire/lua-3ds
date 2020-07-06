function errfunc ()
    error("this is the error message")
end

function msgh (error) 
    print("called error handler with error " .. error)
end

xpcall(errfunc, msgh)