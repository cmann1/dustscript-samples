/*
 * Demostrates how to override built in sounds, and play
 * persistent sound streams that will continue to play
 * after restarts.
 */

const string EMBED_music = "samples/files/music.ogg";
const string EMBED_quack = "samples/files/quack.ogg";

class script
{
	
	scene@ g;
	audio@ music1;
	audio@ music2;

	script()
	{
		@g = get_scene();
		
		// Override dustman light attack sfx with a script sound
		g.override_sound("sfx_dm_attack_light_1", 'quack', true);
		g.override_sound("sfx_dm_attack_light_2", 'quack', true);
		g.override_sound("sfx_dm_attack_light_3", 'quack', true);
		// Replace dustgirl heavy attack sfx with a built in sound
		g.override_sound("sfx_dg_attack_heavy_1", 'sfx_barrel_land', false);
		g.override_sound("sfx_dg_attack_heavy_2", 'sfx_barrel_land', false);
	}
	
	void build_sounds(message@ msg)
	{
		msg.set_string("music", "music");
		msg.set_int("music|loop", 0xffffff);
		msg.set_string("quack", "quack");
	}
	
	/* 
	 * Start persistent streams in on_level_start, after build_sounds.
	 */
	void on_level_start()
	{
		// Play custom music.
		@music1 = g.play_persistent_stream('music', 1, true, 1, true);
		// Play built in music
		@music2 = g.play_persistent_stream('9-bit Expedition', 1, true, 1, false);
		
		// If a persistent stream is already playing, play_persistent_stream
		// won't start a new sound but will instead return the existing audio object
		
		// Do stuff with it if you want
		if(@music1 != null)
			music1.time_scale(0.5);
		if(@music2 != null)
			music2.time_scale(2);
	}
	
	void editor_step()
	{
		// Click in the editor to stop the custom music.
		// scene.stop_persistent_stream must be used instead of audio.stop()
		if(g.mouse_state(0) & 4 != 0)
		{
			g.stop_persistent_stream('music');
		}
		
		// Right click
		if(g.mouse_state(0) & 8 != 0)
		{
			g.stop_persistent_stream('9-bit Expedition');
		}
	}
	
}
