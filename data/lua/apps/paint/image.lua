local Image = {}

-- just hardcoding this lvgl constant like a fool
-- luavgl doesn't expose it
local LV_IMG_CF_TRUE_COLOR = 0x02

function Image:new(image)
    assert(type(image.size) == "number", "Image size must be a number")
    assert(type(image.palette) == "table", "Image palette must be a table")
    assert(type(image.data) == "string", "Image data must be a string")
    assert(image.data:len() == image.size * image.size, "Image data must be the same size as the image")

    for i = 1, image.size * image.size do
        assert(image.palette[tonumber(image.data:sub(i, i))], "Image data must be a valid palette index (at " .. i .. ": " .. image.data:sub(i, i) .. ")")
    end

    setmetatable(image, self)
    self.__index = self
    return image
end

-- note: canvas or image object not supported currently in luavgl; therefore this is literally just a bunch of objects
-- awful for the stack but it works for now lol
function Image:draw(object)
    assert (object.width == object.height, "Can only draw onto a square object")
    assert (object.width % self.size == 0, "Object dimension must be a multiple of image dimension (" .. self.size .. ")")

    local scale = object.width / self.size
    local pixels = {}

    for i = 1, self.size do
        for j = 1, self.size do
            local index = j + (i - 1) * self.size
            object:Object {
                x = (i - 1) * scale,
                y = (j - 1) * scale,
                w = scale,
                h = scale,
                bg_color = self.palette[tonumber(self.data:sub(index, index))],
                radius = 0,
                border_width = 0,
            }:clear_flag(lvgl.FLAG.SCROLLABLE)
        end
    end
end

local function to_rgb565(r, g, b)
    local r5 = math.floor(r * 31 / 255)
    local g6 = math.floor(g * 63 / 255)
    local b5 = math.floor(b * 31 / 255)
    return (r5 << 11) | (g6 << 5) | b5
end

function Image:to_lvgl_imgdesc()
    local function to_rgb565(r, g, b)
        local r5 = math.floor(r * 31 / 255)
        local g6 = math.floor(g * 63 / 255)
        local b5 = math.floor(b * 31 / 255)
        return (r5 << 11) | (g6 << 5) | b5
    end

    local pixels = {}

    for j = 1, self.size do
        for i = 1, self.size do
            local index = (j - 1) * self.size + i
            local palette_idx = tonumber(self.data:sub(index, index))
            local color = self.palette[palette_idx]

            if not color then
                error("Invalid palette index at "..index..": "..palette_idx)
            end

            local r = color.red or 0
            local g = color.green or 0
            local b = color.blue or 0

            local packed = to_rgb565(r, g, b)

            table.insert(pixels, string.pack("<H", packed))
        end
    end

    local data = table.concat(pixels)

    return {
        header = {
            cf = LV_IMG_CF_TRUE_COLOR,
            w = self.size,
            h = self.size,
        },
        data_size = #data,
        data = data,
    }
end

return Image