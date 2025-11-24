\ LP format - for model browsing. Use MPS format to capture full model detail.arc_4-from_Plan_0-left_to_Plan_1-variant_1
Minimize
	1 arc_0-from_Plan_0-left_to_Plan_1-variant_1-right
	+ 3 arc_1-from_Plan_1-left_to_Plan_0-right
   + 5 arc_9-from_Vehicle_0_to_Plan_0-right
   + 2 arc_10-from_Vehicle_0_to_Plan_1-variant_0-right
Subject To
 flow_conservation-Source: arc_2-from_Source_to_Plan_0-left 
   + arc_3-from_Source_to_Plan_1-left + arc_8-from_Source_to_Vehicle_0 = 2
 flow_conservation-Sink: - arc_6-from_Plan_0-right_to_Sink
   - arc_7-from_Plan_1-right_to_Sink = -2
 flow_conservation-Plan_0-right: arc_6-from_Plan_0-right_to_Sink
   - arc_9-from_Vehicle_0_to_Plan_0-right = 0
 flow_conservation-Plan_1-right: arc_7-from_Plan_1-right_to_Sink
   - arc_4-from_Plan_1-variant_0-right_to_Plan_1-right
   - arc_5-from_Plan_1-variant_1-right_to_Plan_1-right = 0
 flow_conservation-Plan_1-variant_0-right:
   arc_4-from_Plan_1-variant_0-right_to_Plan_1-right
   - arc_10-from_Vehicle_0_to_Plan_1-variant_0-right = 0
 flow_conservation-Plan_0-left: - arc_2-from_Source_to_Plan_0-left 
   + arc_0-from_Plan_0-left_to_Plan_1-variant_1-right = 0
 flow_conservation-Plan_1-variant_1-right:
   arc_5-from_Plan_1-variant_1-right_to_Plan_1-right
   - arc_0-from_Plan_0-left_to_Plan_1-variant_1-right = 0
 flow_conservation-Plan_1-left: - arc_3-from_Source_to_Plan_1-left
   + arc_1-from_Plan_1-left_to_Plan_0-right = 0
 flow_conservation-Vehicle_0: - arc_8-from_Source_to_Vehicle_0
   + arc_9-from_Vehicle_0_to_Plan_0-right
   + arc_10-from_Vehicle_0_to_Plan_1-variant_0-right = 0
 variant_consistency-plan_1-variant_0:
   arc_4-from_Plan_1-variant_0-right_to_Plan_1-right
   - arc_3-from_Source_to_Plan_1-left >= 0
Bounds
Binaries
 arc_0-from_Plan_0-left_to_Plan_1-variant_1-right
 arc_1-from_Plan_1-left_to_Plan_0-right
 arc_2-from_Source_to_Plan_0-left 
 arc_3-from_Source_to_Plan_1-left
 arc_4-from_Plan_1-variant_0-right_to_Plan_1-right
 arc_5-from_Plan_1-variant_1-right_to_Plan_1-right
 arc_6-from_Plan_0-right_to_Sink 
 arc_7-from_Plan_1-right_to_Sink
 arc_8-from_Source_to_Vehicle_0 
 arc_9-from_Vehicle_0_to_Plan_0-right
 arc_10-from_Vehicle_0_to_Plan_1-variant_0-right
End
