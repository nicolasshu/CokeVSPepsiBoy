Before this can build you need to do these steps:

1. Create new Folder in Urho3D>Source>Samples called PepsiBoy
2. Now open the CMakeLists,txt in the "Samples" Folder. 1 Above PepsiBoy.
3. Modify the list IF statement with DIR in it to be this:
    if (DIR STREQUAL "PepsiBoy") //Pretty simple eh?
4. You now try and cmake ./ and then make in the top Urho3D directory.

NOTE: The CMakeLists.txt in this Folder is now how we are going to build our PepsiBoy
      binary. So it's in git and able for all of us to edit now.

