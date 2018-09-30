# AshesOfArour

Concept piece for 3rd person action game. Using ray casting, maintain 20 fps on the Gamebuino Classic.

## Work Done
* Wrote assembler functions for fast 32-bit fixed-point multiplaction and division.
* Walls, floors and ceiling rendered.
* Wall rendering produces nasty saw-toothed effects up close - removed them by calculating y-draw step from the centre of the screen instead for from the top.
* Had to keep and embed texture/sprite column bliting in the code to keep performance up

## To Do
* Rendering functions can be faster if written in assembler, but that is a big job.
* Finish sprite bliting
* Collision detection
* Code logic :)
