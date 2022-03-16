## Ideas:
- [ ] step through history to refine the start and endpoints ([remove from front of  (and end of) vector](https://stackoverflow.com/questions/7351899/remove-first-n-elements-from-a-stdvector)) 
 - [ ] Identify positions to be human, bot, or disabled entirely
	 - [ ]  Can probably start by loading a drill, pausing (or constantly setting current position), listing identifiers along with player names and using a couple t/f checkboxes to determine
 - [ ] IMGUI representation
	 - [ ] field representation and colored markers of player and ball position
	 - [ ] allow for position checkboxes to be human, bot, or disabled entirely
	 - [ ] adjust boost amount at end position..?
	 - [ ] allow for re-ordering like below
 - [ ] Re-order Drill positions for new teammate matchups (because server car vector is grabbed new every time)

## Todo:
 - [x] keep track of and increment rep number
 - [x] Mirror halfway through
 - [ ] Handle extra player spawns
 - [ ] Handle too little player spawns
 - [ ] function button bindings
 - [ ] next drill, end this drill execution, reset drill with previous layout (cycle players backwards) 
 - [ ] Change pack name
	 - [ ] Error check file name (no spaces? not too long?) (or just remove spaces, lowerCamelCase, and truncate)
 - [ ] swap in folderPath to saving/loading files