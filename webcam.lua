-- webcam.lua - Reusable webcam library module for LÖVE2D
-- Usage:
--   local webcam = require("webcam")
--   local cam = webcam.new("/dev/video0", 640, 480)
--   
--   -- In love.update:
--   cam:update()
--   
--   -- In love.draw:
--   cam:draw(x, y, scale)

local ffi = require("ffi")

-- Load the C library
local webcam_lib = ffi.load("./webcam.so")

-- FFI declarations
ffi.cdef[[
    typedef struct webcam_t webcam_t;
    
    webcam_t* webcam_open(const char *device, int width, int height);
    const void* webcam_capture(webcam_t *cam);
    int webcam_get_width(webcam_t *cam);
    int webcam_get_height(webcam_t *cam);
    size_t webcam_get_buffer_size(webcam_t *cam);
    void webcam_close(webcam_t *cam);
]]

-- Webcam class
local Webcam = {}
Webcam.__index = Webcam

-- Create a new webcam instance
function Webcam.new(device, width, height)
    device = device or "/dev/video0"
    width = width or 640
    height = height or 480
    
    local self = setmetatable({}, Webcam)
    
    -- Open webcam
    self.cam = webcam_lib.webcam_open(device, width, height)
    
    if self.cam == nil then
        error("Failed to open webcam device: " .. device)
    end
    
    -- Get actual dimensions
    self.width = webcam_lib.webcam_get_width(self.cam)
    self.height = webcam_lib.webcam_get_height(self.cam)
    
    -- Create ImageData and texture
    self.imageData = love.image.newImageData(self.width, self.height)
    self.texture = nil
    self.lastFrame = nil
    
    return self
end

-- Update - capture a new frame
function Webcam:update()
    if not self.cam then return false end
    
    -- Capture frame
    local frame_ptr = webcam_lib.webcam_capture(self.cam)
    
    if frame_ptr ~= nil then
        -- Cast to uint8_t array
        local pixels = ffi.cast("uint8_t*", frame_ptr)
        
        -- Copy RGB pixels to ImageData
        local idx = 0
        for y = 0, self.height - 1 do
            for x = 0, self.width - 1 do
                local r = pixels[idx] / 255
                local g = pixels[idx + 1] / 255
                local b = pixels[idx + 2] / 255
                self.imageData:setPixel(x, y, r, g, b, 1)
                idx = idx + 3
            end
        end
        
        -- Update texture
        if self.texture then
            self.texture:release()
        end
        self.texture = love.graphics.newImage(self.imageData)
        
        return true
    end
    
    return false
end

-- Draw the webcam feed
function Webcam:draw(x, y, scale, rotation)
    if not self.texture then return end
    
    x = x or 0
    y = y or 0
    scale = scale or 1
    rotation = rotation or 0
    
    love.graphics.draw(self.texture, x, y, rotation, scale, scale)
end

-- Draw scaled to fit a rectangle
function Webcam:drawFit(x, y, width, height)
    if not self.texture then return end
    
    local scaleX = width / self.width
    local scaleY = height / self.height
    local scale = math.min(scaleX, scaleY)
    
    local drawX = x + (width - self.width * scale) / 2
    local drawY = y + (height - self.height * scale) / 2
    
    love.graphics.draw(self.texture, drawX, drawY, 0, scale, scale)
end

-- Get the texture
function Webcam:getTexture()
    return self.texture
end

-- Get dimensions
function Webcam:getDimensions()
    return self.width, self.height
end

function Webcam:getWidth()
    return self.width
end

function Webcam:getHeight()
    return self.height
end

-- Check if webcam is active
function Webcam:isActive()
    return self.cam ~= nil
end

-- Close the webcam
function Webcam:close()
    if self.cam then
        webcam_lib.webcam_close(self.cam)
        self.cam = nil
    end
    
    if self.texture then
        self.texture:release()
        self.texture = nil
    end
end

-- Cleanup on garbage collection
function Webcam:__gc()
    self:close()
end

-- Module exports
return {
    new = Webcam.new,
    version = "1.0.0"
}
