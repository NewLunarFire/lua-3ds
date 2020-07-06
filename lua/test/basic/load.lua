local chunk_pieces = {"print(", "\"This is loaded", " from a function\"", ")"}
local current_piece = 0

function loadchunk ()
    current_piece = current_piece + 1
    return chunk_pieces[current_piece]
end

local func1 = load("print(\"Load a chunk from a string\")")
func1()

local func2 = load(loadchunk)
func2()


