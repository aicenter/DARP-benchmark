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

