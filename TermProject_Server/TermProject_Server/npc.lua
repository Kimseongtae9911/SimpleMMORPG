myid = 99999;
function monster_initialize(x)
	myid = x;
	API_Initialize(myid);
end

function event_player_move(player)
	player_x = API_get_x(player);
	player_y = API_get_y(player);
	my_x = API_get_x(myid);
	my_y = API_get_y(myid);
	if (player_x == my_x) then
		if (player_y == my_y) then
			API_SendMessage(myid, player, "HELLO");
		end
	end
end