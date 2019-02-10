# Level editor.

#### Main controls:
Holding **SHIFT** lets you control the camera  
`Drag with Left Mouse button`: rotate camera  
`Drag with Right mouse button`: camera dolly mode (move forward and back)  
`Drag with Middle mouse button`: moves camera up-down and left-right  
`Rotating Mouse Wheel`: changes camera movement speed  

Other controls:  
`Ctrl + Z`: Undo action (only works with Height field editor for now)  

### Height field editor:

##### Controls:
`Space`: rotate texture for placement  
`Control + Mouse wheel`: Changes radius of tool  
`Left mouse`: sets tile texture and properties (see "What paint" and "Tile flags")  
`Right mouse`: deletes tile texture, keeps properties  
`Middle mouse drag`: draws a line and performs same as left mouse for each tile in line  
	
`Holding Control`: changes height of tiles, pressing left, right and middle buttons behaves as above for height (see the Height Paint settings)  
`Holding ALT`: On left click copies Tile properties, including tile height   
	
UI explanation:  
`"LAYER"`: There is 4 layers of heightfields so you can create something like bridges, or water.  
 - Please note the AI Roads are forced to be at LAYER 0  

`"Draw helpers"` shows you what kind of tiles are painted on region. Color shows what tile flags set  

Tile flags defines how geometry of heightfield is generated:  
- Detached (bad name actually) - tiles will not be sloped to surrounding tiles of non-detached 
- Add wall - adds a wall to the detached tiles  
- Wall collision - pretty self explanatory  
- No collide - this tiles has no collision at all  
	
### Level objects:  
`T` changes placement mode (Tile or Free)  
`Space`: rotate 90 degrees for placement  

`Left click`: places object  
`Right click`: removes object (if it's placed on Tile)  
	
`Control + Left click`: toggle selection  
`Control + Space`: duplicate selection  
`ESC`: cancel selection  
`DEL`: deletes selection  
`G`: movement mode  
`R`: rotation mode  
`N`: set the name of object (for scripting purposes)  
	
### Building constructor:  
It has to be reworked.  
Basically works just like Level objects editor.  
But if you want to create new building, you have to create models for templates and place variants.  
	
### Occluder editor:  
This is important tool because you can seriously drop performance with your cities.  

Manipulation:
`Control + Left click`: toggle selection  
`ESC`: cancel selection  
`DEL`: deletes selection  

Placing:  
`First left click`: begins occluder  
`Second left click`: sets end point  
`Third left click`: sets the height of occluder and ends it.  
	
### Road editor:

`Holding Left Mouse`: paints waypoints  
`Holding Right mouse`: removes waypoints  
`Holding Middle mouse`: draws a line  
`Space`: rotate 90 degrees for placement  
`T`: cycles between types  
	
### Prefab manager:  
This tool still not completely done, it must be able to save/place object and occluders  

Placement mode:  
`Double click` in list to select prefab  
`Space`: rotate 90 degrees for placement  
`ESC`: cancel placement  
		
Creation mode:  
Just select the region by `dragging mouse with left button pressed`, and press `Create` in the UI  