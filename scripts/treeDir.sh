echo "This is the Wankel Engine Dir structure excluding all of the contents of the external library dirs" > Documents/DirStructure.txt
tree -L 1 >> Documents/DirStructure.txt
tree -L 1 external >> Documents/DirStructure.txt
tree src >> Documents/DirStructure.txt
tree SandboxApp >> Documents/DirStructure.txt
tree scripts >> Documents/DirStructure.txt
