local TinyImage = {}
local tinyimage_c = require("tinyimage")

-- Create a new TinyImage
function TinyImage:new(width, height)
    local image = {}
    image.width = width
    image.height = height
    image.data = tinyimage_c.new(width, height)

    setmetatable(image, self)
    self.__index = self
    return image
end

-- function TinyImage:set_pixel(x, y, color)
--     return tinyimage_c.set_pixel(self, x, y, color)
-- end

-- function TinyImage:get_pixel(x, y)
--     return tinyimage_c.get_pixel(self, x, y)
-- end

-- function TinyImage:set_palette(index, r, g, b)
--     return tinyimage_c.set_palette(self, index, r, g, b)
-- end

-- function TinyImage:get_palette(index)
--     return tinyimage_c.get_palette(self, index)
-- end

-- function TinyImage:get_size()
--     return tinyimage_c.get_size(self)
-- end

-- function TinyImage:to_image()
--     return tinyimage_c.to_image(self)
-- end

return TinyImage 