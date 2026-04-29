echo "This is the Wankel Engine Dir structure excluding all of the contents of the external library dirs" > DirStructure.txt
tree -L 1 >> DirStructure.txt
tree -L 1 external >> DirStructure.txt
tree src >> DirStructure.txt
tree SandboxApp >> DirStructure.txt
tree scripts >> DirStructure.txt
