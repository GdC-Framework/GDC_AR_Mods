# MM library proxy used (GDC)

English | [Français](gdc-mm-library-proxy.fr.md)

Rule 2.2 · error · GDC convention

## What happens
The items of the Prefab Library (PrefabLibrary/MM Forces, MM Markers, MM Systems) are proxies: empty child prefabs stored in your own GDC Mission Maker Local addon, which is never published. A mission that references a proxy needs that addon: on the server and for the players, the entity or the property points to nothing.

## How it happens
Dropping an item into the viewport is safe: MM Tools places the real prefab. A proxy picked or pasted in a resource property (spawner, deleter filter...) is not caught.

## What to do
- Entity: delete it, then drop the item again from the Prefab Library into the viewport.
- Property: pick the real prefab in the Resource Browser.

## History
- Davay: a proxy referenced in the property of a "Bloc" entity.
