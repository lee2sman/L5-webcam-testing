require("L5")
local webcam = require("webcam")

local cam
local clicks=1
function setup()
    print("Webcam Library v" .. webcam.version)
    
    -- Create webcam instance
    cam = webcam.new("/dev/video0", 640, 480)
    
    print(string.format("Webcam opened: %dx%d", cam:getWidth(), cam:getHeight()))

    windowTitle("webcam test")
    describe("Turns on camera and draws to window.")
end

function draw()
    if cam then
        cam:update()
        -- Option 1: Draw at specific position with scale
        -- cam:draw(0, 0, 1)
        
        -- Option 2: Draw scaled to fit window
        cam:drawFit(0, 0, width, height)
    else
        text("No webcam available", 10, 10)
    end

    if clicks==2 then
    filter(POSTERIZE)
  elseif clicks==3 then
    filter(THRESHOLD)
  end
    
    -- Show FPS
    fill("green")
    text("FPS: " .. love.timer.getFPS(), 10, 10)
    fill('white')

end

function love.quit()
    if cam then
        cam:close()
    end
end

function keyPressed()
    if key == "escape" then
        love.event.quit()
    end
end

function mousePressed()
  clicks=clicks+1
end
