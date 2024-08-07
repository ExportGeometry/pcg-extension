# PCG Extension
This repo collects my public PCG tools



## Supported Versions

* Unreal Engine 5.4



## Installation

1) Make sure that you have installed a supported version of **Unreal Engine**.
   1) See the "Supported Versions" section above.
2) Create a C++ UE project.
3) Add the PCGExtension plugin to your UE project's Plugins/ folder.
4) Regenerate visual studio project files.
   1) Close your IDE, go to your .uproject file, right click, and select "generate visual studio project files".
5) Build and run your project.



## Tools

### Custom PCG Nodes

#### ExtractCollisionBoxes

This node allows you to create a custom collision box for a mesh and extract the data for this collision box inside PCG. This can be useful if you want to cull meshes based on a specific section of the mesh intersecting with some other object. For example, if you have populated a bunch of mushroom meshes across a surface and you want to cull them based on if the mushroom cap is intersecting with anything (but ignore intersections of the mushroom stalk).



##### ExtractCollisionBoxes: Short Walkthrough

1) Pick a mesh that you want to extract collision data from.
2) Open the mesh asset editor and select **Collision -> Add Box Simplified Collision** from the top toolbar to add a collision box. 
3) Move and scale the collision box as needed.
   1) If you don't see your collision box for some reason, check that you have **Show -> Simple Collision** turned on.
4) (Optional) Name your collision box.
   1) By default, the ExtractCollisionBoxesNode will just grab the first collision box it finds. If you have multiple collision boxes attached to your mesh, you can select one by name.
   2) In the details panel for your mesh, go to **Collision -> Primitives -> Boxes -> Index [n] -> Name** and change the name field.
5) (Optional) Repeat steps 1-4 for as many mesh variants as you want.
6) Create a DataAsset of type **PCGEMeshData**.
7) Add a reference to each mesh you wish to extract collision from to your DataAsset.
8) In your PCG graph, add a new parameter of type **PCGEMeshData** and reference the data asset you just created.
9) **Right Click in Graph -> Get YOUR_PARAM** to get the parameter you just created.
10) Wire this to a **GetPropertyFromObjectPathNode** and set the **PropertyName** field to "WeightedMeshData"
11) Wire this to the **ExtractCollisionBoxes** node and set the **MeshAttribute** field to "Mesh"
12) If you inspect the PCG graph (while an active version of your PCG graph exists in the current scene), you should see one point for each extracted collision box. 
