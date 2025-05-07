local TinyImage = {}
local tinyimage_c = require("tinyimage")

-- Create a new TinyImage
function TinyImage:new(o)
    local image = o or {}
    image.width = o.width
    image.height = o.height
    
    -- Create the userdata
    image.data = tinyimage_c.new(o.width, o.height)
    
    -- Set up the wrapper table's metatable
    setmetatable(image, {__index = TinyImage})
    
    if (image.palette) then 
        assert(#image.palette == 4, "Palette must be a table with 4 colors")
        for i = 1, 4 do
            assert(type(image.palette[i]) == "string" and #image.palette[i] == 7, "Colour must be a hex string")
            image:set_palette(
                i - 1, 
                tonumber(image.palette[i]:sub(2, 3), 16), 
                tonumber(image.palette[i]:sub(4, 5), 16), 
                tonumber(image.palette[i]:sub(6, 7), 16)
            )
        end
    end
    return image
end

function TinyImage:set_pixel(x, y, color)
    return tinyimage_c.set_pixel(self.data, x, y, color)
end

function TinyImage:get_pixel(x, y)
    return tinyimage_c.get_pixel(self.data, x, y)
end

function TinyImage:set_palette(index, r, g, b)
    return tinyimage_c.set_palette(self.data, index, r, g, b)
end

function TinyImage:get_palette(index)
    return tinyimage_c.get_palette(self.data, index)
end

function TinyImage:get_size()
    return tinyimage_c.get_size(self.data)
end

-- function TinyImage:to_image()
--     return tinyimage_c.to_image(self.data)
-- end

function TinyImage:draw(object)
    width, height = self:get_size()
    local scale = object.width / width

    assert(math.floor(scale) == scale, "Object scale must be an integer")
    assert(object.height / height == scale, "Object scale must be consistent in x and y")

    for i = 1, width do
        for j = 1, height do
            object:Object {
                x = (i - 1) * scale,
                y = (j - 1) * scale,
                w = scale,
                h = scale,
                bg_color = string.format("#%02x%02x%02x", self:get_palette(self:get_pixel(i - 1, j - 1))),
                radius = 0,
                border_width = 0,
            }:clear_flag(lvgl.FLAG.SCROLLABLE)
        end
    end
    
end

return TinyImage 