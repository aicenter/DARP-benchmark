<!--
MIT License

Copyright (c) 2026 Czech Technical University in Prague

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.-->
# Data-related problems

## Demend in private areas

Possible solutions:
- **add private roads to the map**
    - should be easy
    - needs map reimport
    - realistic: taxi can access private area if customers are there
    - unrealistic: taxi can also pass throught
- ignore demand in private areas
    - unrealistic
    - even less demand
    - requires a manual banlist either in the position generation script or in db
- banned zones
    - ban zones that does not have an intersection with ways
    - can be done automatically but requires a manual check (the road network can be problematic)


## Zones without nodes 

Possible solutions:
- **assign ways instead nodes**
    - join way, then randomly choose one of the two nodes
    - no errors hidden
    - doesnt require two-step assignment
- if there are not any nodes assign nearest nodes and check if distance is not too big
    - requires a two step assignment
    - can hide some errors
    - randomness lost if no node is present
- if there are not any nodes, enlarge the zone using st buffer 	
    - requires a two step assignment
    - can hide some errors
- merge zones into larger units
    - unnecesary loss of precision

