/*
 * Demonstrates how to use collision callbacks to create custom geometry
 */

// Requires https://github.com/cmann1/DustScripts
#include '../lib/std.cpp';
#include '../lib/enums/GVB.cpp';
#include '../lib/math/math.cpp';
#include '../lib/math/Line.cpp';
#include '../lib/input/Mouse.cpp';

const array<string> SURFACE_TYPES = {
	'none', 'stone', 'dirt', 'metal', 'stone', 'grass',
	'water', 'wood', 'wood', 'dirt', 'carpet', 'poly' };

enum SurfaceSide { Ground, Roof, WallLeft, WallRight, }

enum side_types {
  side_left = 0,
  side_right = 1,
  side_roof = 2,
  side_ground = 3,
}

// For editing lines
const float MOUSE_PICK_DIST = 20;
const array<uint> SIDE_COLOURS = {
	0xffff5555, 0xffff9999, // Ground
	0xffff55ff, 0xffff99ff, // Roof
	0xff55ff55, 0xff99ff99, // WallLeft
	0xff5555ff, 0xff9999ff, // WallRight
};

const float COLLISION_OFFSET = 10;

class script : callback_base
{

	[text] array<Surface> surfaces;

	scene@ g;
	editor_api@ editor;
	Mouse mouse(false);
	controllable@ c;

	float t;
	Line line;
	Line c_line;
	float ox1, oy1;
	float ox2, oy2;
	int drag_index = -1;
	int drag_vertex = 0;
	int collision_index = -1;

	int hover_index = -1;
	int hover_vertex = 0;

	bool editor_initialised;

	script()
	{
		@g = get_scene();
		@editor = get_editor_api();

		surfaces.resize(1);
		surfaces[0].x1 = -100;
		surfaces[0].y1 = -55;
		surfaces[0].x2 =  100;
		surfaces[0].y2 = -55;
	}
	
	void on_level_start()
	{
		@c = controller_controllable(0);

		if(@c != null)
		{
			// Set up custom collision handling functions
			c.set_collision_handler(this, 'collision_handler', 0);
			c.set_texture_type_handler(this, 'texture_type_handler', 0);

			// Slope min/max, slant min/max angles.
			// Required for non-45deg slope sliding
			c.set_ground_angles(40, 50, 10, 40);
			// Slant down facing min/max, slant up facing min/max angles
			// Required or walls > 116deg won't collide
			c.set_wall_angles(116, 134, 51, 89);
			// Roof slope min/max, roof slant min/max angles
			// Required to make the player rotate on ceilings > 154
			c.set_roof_angles(126, 153, 154, 160);

			dustman@ dm = c.as_dustman();

			if(@dm != null)
			{
				dm.on_subframe_end_callback(this, 'on_subframe_end', 0);
			}
		}

		init();
	}

	void init()
	{
		for(uint i = 0; i < surfaces.length; i++)
		{
			surfaces[i].update();
		}
	}

	/* Not important - code for finding line under mouse */
	void find_line()
	{
		float tx, ty;
		float closest_dist = 999999;

		hover_index = -1;
		hover_vertex = 0;

		for(uint i = 0; i < surfaces.length; i++)
		{
			Surface@ surface = @surfaces[i];
			line.x1 = surface.x1;
			line.y1 = surface.y1;
			line.x2 = surface.x2;
			line.y2 = surface.y2;
			const float t = line.closest_point(mouse.x, mouse.y, tx, ty);
			const float dist = dist_sqr(mouse.x, mouse.y, tx, ty);

			if(dist < closest_dist && dist <= MOUSE_PICK_DIST * MOUSE_PICK_DIST)
			{
				closest_dist = dist;
				hover_index = int(i);

				if(dist_sqr(mouse.x, mouse.y, line.x1, line.y1) <= MOUSE_PICK_DIST * MOUSE_PICK_DIST)
					hover_vertex = -1;
				else if(dist_sqr(mouse.x, mouse.y, line.x2, line.y2) <= MOUSE_PICK_DIST * MOUSE_PICK_DIST)
					hover_vertex =  1;
				else
					hover_vertex =  0;
			}
		}
	}

	/* Not important - code for editing lines */
	void do_edit()
	{
		mouse.step(@editor != null && (editor.key_check_gvb(GVB::Space) || editor.editor_tab() != 'Scripts' || editor.mouse_in_gui()));

		const bool snap =
		@c != null && c.light_intent() != 0 ||
		@editor != null && editor.key_check_gvb(GVB::Shift);

		find_line();

		if(hover_index != -1)
		{
			Surface@ surface = @surfaces[hover_index];

			if(mouse.left_press)
			{
				ox1 = surface.x1 - mouse.x;
				oy1 = surface.y1 - mouse.y;
				ox2 = surface.x2 - mouse.x;
				oy2 = surface.y2 - mouse.y;

				drag_index = hover_index;
				drag_vertex = hover_vertex;
			}
			else if(mouse.middle_press)
			{
				surface.material_index = (surface.material_index + 1) % int(SURFACE_TYPES.length);
				puts(' changing surface: ' + SURFACE_TYPES[surface.material_index]);
			}
			else if(mouse.right_press)
			{
				surfaces.removeAt(hover_index);
				hover_index = -1;
				hover_vertex = 0;
				drag_index = -1;
			}
		}

		// Create
		if(mouse.left_press && hover_index == -1 && drag_index == -1)
		{
			drag_index = int(surfaces.length);
			drag_vertex = 1;
			surfaces.insertLast(Surface());
			Surface@ surface = @surfaces[drag_index];
			surface.x1 = mouse.x;
			surface.y1 = mouse.y;
			surface.x2 = mouse.x;
			surface.y2 = mouse.y;
			ox1 = oy1 = ox2 = oy2 = 0;

			if(snap)
			{
				surface.snap(-1);
				surface.snap( 1);
				surface.update();
			}
		}

		if(drag_index != -1)
		{
			Surface@ surface = @surfaces[drag_index];

			if(drag_vertex <= 0)
			{
				surface.x1 = mouse.x + ox1;
				surface.y1 = mouse.y + oy1;

				if(snap) surface.snap(-1);
			}

			if(drag_vertex >= 0)
			{
				surface.x2 = mouse.x + ox2;
				surface.y2 = mouse.y + oy2;

				if(snap)
					surface.snap(1);
			}

			surface.update();

			if(!mouse.left_down)
			{
				drag_index = -1;
			}
		}
	}

	void step(int)
	{
		collision_index = -1;

		do_edit();
	}

	void editor_step()
	{
		if(!editor_initialised)
		{
			init();
			editor_initialised = true;
		}

		step(0);
	}

	void draw(float sub_frame)
	{
		const int highlight_index = drag_index != -1 ? drag_index : hover_index;
		const int highlight_vetex = drag_index != -1 && drag_vertex != -1 ? drag_vertex : hover_vertex;

		for(uint i = 0; i < surfaces.length; i++)
		{
			Surface@ surface = @surfaces[i];
			const bool highlight = int(i) == highlight_index;

			g.draw_line_world(
				highlight ? 22 : 19,
				10,
				surface.x1, surface.y1, surface.x2, surface.y2, 4, 
				SIDE_COLOURS[surface.side * 2 + (highlight ? 1 : 0)]);

			if(!is_playing())
			{
				const float mx = (surface.x1 + surface.x2) * 0.5;
				const float my = (surface.y1 + surface.y2) * 0.5;
				g.draw_line_world(
				19, 10,
				mx, my, mx + surface.nx * 20, my + surface.ny * 20, 2, 0xaaffffff);
			}
		}

		if(highlight_vetex != 0 && highlight_index != -1)
		{
			Surface@ surface = @surfaces[highlight_index];
			const float x = highlight_vetex == -1 ? surface.x1 : surface.x2;
			const float y = highlight_vetex == -1 ? surface.y1 : surface.y2;
			g.draw_rectangle_world(22, 11, x - 4, y - 4, x + 4, y + 4, 45, 0xffaa66ff);
		}
	}

	void editor_draw(float sub_frame)
	{
		draw(sub_frame);
	}

	/*
	 * Generic method for finding only collisions of a specific surface side/direction
	 */
	bool check_collision_side(const SurfaceSide side, tilecollision@ t)
	{
		float x, y, dt;

		for(uint i = 0; i < surfaces.length; i++)
		{
			Surface@ surface = @surfaces[i];

			if(surface.side != side)
				continue;

			line.x1 = surface.x1;
			line.y1 = surface.y1;
			line.x2 = surface.x2;
			line.y2 = surface.y2;

			if(line.intersection(c_line, x, y, dt))
			{
				t.hit(true);
				t.type(surface.angle);
				t.hit_x(x);
				t.hit_y(y);
				collision_index = i;
				return true;
			}
		}

		return false;
	}

	// Collision handlers:
	// --------------------------------------------
	
	/*
	 * Allows footstep and sliding sounds to be played for custom collisions
	 */
	void texture_type_handler(controllable@ c, texture_type_query@ query, int)
	{
		if(collision_index == -1)
			return;

		// Special texture_type_query for passing values back and forth.
		// x(), y(), top_surface() returns the position and if the query is for a ground surface
		// Pass result() the desired texture type
		query.result(SURFACE_TYPES[surfaces[collision_index].material_index]);
	}

	void collision_handler(controllable@ c, tilecollision@ t, int side, bool moving, float snap_offset, int)
	{
		switch(side)
		{
			case side_ground:
			{
				// Small platform at bottom
				if(c.x() > 96 && c.x() < 288 && c.y() > -48 && c.y() < 5)
				{
					t.hit(true);
					t.type(0);
					t.hit_x(c.x());
					t.hit_y(-21);
					return;
				}

				c_line.x2 = c.x();
				c_line.y2 = c.y() + COLLISION_OFFSET + snap_offset;
				c_line.x1 = c_line.x2;
				c_line.y1 = c_line.y2 - 48;
				
				// Perform the built in tile collision check
				if(!check_collision_side(SurfaceSide::Ground, t))
				{
					c.check_collision(t, side, moving, snap_offset);
				}
				
				break;
			}
			case side_roof:
			{
				c_line.x2 = c.x();
				c_line.y2 = c.y() - 48;
				c_line.x1 = c_line.x2;
				c_line.y1 = c_line.y2 - 48 - COLLISION_OFFSET - snap_offset;
				
				// Perform the built in tile collision check
				if(!check_collision_side(SurfaceSide::Roof, t))
				{
					c.check_collision(t, side, moving, snap_offset);
				}
				break;
			}
			case side_left:
			{
				c_line.x1 = c.x();
				c_line.y1 = c.y() - 48;
				c_line.x2 = c.x() - 24 - COLLISION_OFFSET - snap_offset;
				c_line.y2 = c_line.y1;
				
				// Perform the built in tile collision check
				if(!check_collision_side(SurfaceSide::WallRight, t))
				{
					c.check_collision(t, side, moving, snap_offset);
				}
				
				break;
			}
			case side_right:
			{
				c_line.x1 = c.x();
				c_line.y1 = c.y() - 48;
				c_line.x2 = c.x() + 24 + COLLISION_OFFSET + snap_offset;
				c_line.y2 = c_line.y1;
				
				// Perform the built in tile collision check
				if(!check_collision_side(SurfaceSide::WallLeft, t))
				{
					c.check_collision(t, side, moving, snap_offset);
				}
				
				break;
			}
		}
	}
	
	/*
	 * Not used here, but can be used if finer control over the player's rotation is needed
	 */
	void on_subframe_end(dustman@ dm, int)
	{
	}

}

/*
 * Class holding information about a custom collision surface
 */
class Surface
{

	[text] float x1;
	[text] float y1;
	[text] float x2;
	[text] float y2;
	[text] int material_index = 3;

	SurfaceSide side;
	int angle;
	float nx, ny;

	void update()
	{
		nx =  (y2 - y1);
		ny = -(x2 - x1);
		const float length = magnitude(nx, ny);
		nx = length != 0 ? nx / length : 0;
		ny = length != 0 ? ny / length : -1;

		angle = round_int(atan2(nx, -ny) * RAD2DEG);

		if(angle <= 50 && angle >= -50)
			side = SurfaceSide::Ground;
		else if(angle >= 135 || angle <= -135)
			side = SurfaceSide::Roof;
		else if(angle > 0)
			side = SurfaceSide::WallRight;
		else
			side = SurfaceSide::WallLeft;
	}

	void snap(const int vertex=99)
	{
		if(vertex == -1)
		{
			x1 = round(x1 / 10) * 10;
			y1 = round(y1 / 10) * 10;
		}

		if(vertex ==  1)
		{
			x2 = round(x2 / 10) * 10;
			y2 = round(y2 / 10) * 10;
		}
	}

}
