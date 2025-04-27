local Image = {}

function Image:new(image)
    assert (type(image.size) == "number", "Image size must be a number")
    assert (type(image.data) == "table", "Image data must be a table")

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
            if self.data[j + (i - 1) * self.size] then
                object:Object {
                    x = (i - 1) * scale,
                    y = (j - 1) * scale,
                    w = scale,
                    h = scale,
                    bg_color = self.data[j + (i - 1) * self.size],
                    radius = 0,
                    border_width = 0,
                }:clear_flag(lvgl.FLAG.SCROLLABLE)
            end
        end
    end
end

return Image