t = 1.2;
t_z = 2;
tolerance = 0.99;

hull_x = 90;
hull_y = 45;
hull_z = 11;

$fn=36;

module display()
{
    display_x = 80;
    display_y = 38;
    display_z = 1.5;
    window_x = 69;
    window_y = 32;
    window_z = 0.5;
    window_offset = (display_y-window_y)/2;
    connector_x = 5;
    connector_y = 19;

    translate([connector_x+display_x-window_x-window_offset, window_offset, 0]) cube([window_x, window_y, window_z]);
    translate([connector_x, 0, window_z]) cube([display_x, display_y, display_z]);
    translate([0, (display_y-connector_y)/2, window_z]) cube([connector_x, connector_y, display_z]);
}

module button()
{
    button_x = 4;
    button_y = 5.5;
    thickness = 0.4;
    space = 0.6;

    difference()
    {
        union()
        {
            translate([0, 0, 0]) cube([button_x, button_y, t_z]);
            translate([0, 9, 0]) cube([button_x, button_y, t_z]);
            translate([0, 18, 0]) cube([button_x, button_y, t_z]);
            translate([0, 27, 0]) cube([button_x, button_y, t_z]);  
        }
        
        translate([0, 0+space, 0]) cube([button_x-space, button_y-2*space, thickness]);
        translate([0, 9+space, 0]) cube([button_x-space, button_y-2*space, thickness]);
        translate([0, 18+space, 0]) cube([button_x-space, button_y-2*space, thickness]);
    }  
}

module switch()
{
    switch_x = 10;
    switch_z = 5;

    cube([switch_x, t, switch_z]);
    translate([0, t, -2.5]) cube([switch_x, 4, switch_z+2]);
}

module sd()
{
    sd_x = 16;
    sd_z = 3;

    cube([sd_x, t, sd_z]);
}

module usb()
{
    usb_x = 10;
    usb_z = 5;
    usb_channel = 0.75;

    cube([usb_x, t, usb_z]);
    translate([0, 0, usb_z]) cube([usb_x, usb_channel, 3]);
}

module holder1()
{
    cube([17.5, 1, 1]); //14
    translate([45, 0, 0]) cube([5, 1, 1]);
    translate([56, 0, 0]) cube([3, 1, 1]);
}

module holder2()
{
    cube([9, 1, 1]);
    translate([19, 0, 0]) cube([40, 1, 1]);
}

module letter()
{
    translate([0, -t+t/3, t_z]) rotate([90, 0, 0]) linear_extrude(t/3) text(text="the badge", size=2.5);
    translate([19, -t+t/3, 0.75]) rotate([90, 0, 0]) linear_extrude(t/3) text(text="off / on", size=1.5);
    translate([-t+t/3, 34, t_z]) rotate([270, 270, 90]) linear_extrude(t/3) text(text="reset", size=2.5);
    translate([-t+t/3, 25, t_z]) rotate([270, 270, 90]) linear_extrude(t/3) text(text="1", size=2.5);
    translate([-t+t/3, 16, t_z]) rotate([270, 270, 90]) linear_extrude(t/3) text(text="2", size=2.5);
    translate([-t+t/3, 7, t_z]) rotate([270, 270, 90]) linear_extrude(t/3) text(text="3", size=2.5);   
}

module hull()
{    
    display_offset_x = 5;
    display_offset_y = 3.5;
    button_offset_x = 0;
    button_offset_y = 5;
    switch_offset_x = 17.5;
    switch_offset_z = 3;
    sd_offset_x = 29;
    sd_offset_z = 3;
    usb_offset_x = 9;
    usb_offset_z = 2;
    holder_offset = 3.5;

    difference()
    {
        minkowski()
        {
            cube([hull_x, hull_y, hull_z-1]);
            cylinder(r=t, h=1);
        }
    
        translate([0, 0, t_z]) cube([hull_x, hull_y, hull_z-t_z]);

        translate([display_offset_x, display_offset_y, 0]) display();
        translate([button_offset_x,  button_offset_y, 0]) button();
        translate([switch_offset_x, -t, switch_offset_z]) switch();
        translate([sd_offset_x, -t, sd_offset_z]) sd();
        translate([usb_offset_x, hull_y, usb_offset_z]) usb();
        
        translate([(1-tolerance)/2*hull_x+3, -t, 6.8]) rotate([270, 0, 0]) cylinder(h=t, d=3);
        translate([(1-tolerance)/2*hull_x+3, hull_y, 6.8]) rotate([270, 0, 0]) cylinder(h=t, d=3);
        translate([65+3, -t, 6.8]) rotate([270, 0, 0]) cylinder(h=t, d=3);
        translate([65+3, hull_y,6.8]) rotate([270, 0, 0]) cylinder(h=t, d=3);

        letter();
    }

    translate([0, 0, holder_offset]) holder1();
    translate([0, hull_y-1, holder_offset]) holder2();
}

module border()
{
    height = 1.5;
    
    difference()
    {
        cube([hull_x*tolerance, hull_y*tolerance, height]);
        translate([t, t, 0]) cube([hull_x*tolerance-2*t, hull_y*tolerance-2*t, height]);
    }
}

module pinHole()
{
    pin_hole_x =33;
    pin_hole_y =6.5;

    cube([pin_hole_x, pin_hole_y, t]);
}

module speakerHoles()
{
    radius = 0.5;
    
    for (x =[-4, 0, 4])
    {
        for (y = [-4, 0, 4])
        {
            translate([-radius*x, -radius*y, 0]) cylinder(h=t, r=radius);
        }
    }
    
    translate([-2*radius*4, 0, 0]) cylinder(h=t, r=radius);
    translate([2*radius*4, 0, 0]) cylinder(h=t, r=radius);
}

module separator()
{
    height = 4;
    
    cube([t, hull_y*tolerance, height]);
}

module latch()
{
    height = 6;
    width = 6;
    diameter = 3;
    
    cube([width, t, height]);
    translate([width/2, 0, 4]) rotate([90, 0, 0]) difference()
    {
        scale ([1, 1, 0.3]) sphere(d=diameter);
        translate([0, 0, -diameter]) cube(2*diameter, center= true); 
    }
}

module cover()
{
    pinHole_x = 28;
    pinHole_y = 4;
    
    speaker_x = 45;
    speaker_y = 22;

    separator_offset = 65;
    
    difference()
    {
        minkowski()
        {
            cube([hull_x, hull_y, t-1]);
            cylinder(r=t, h=1);
        }
    
        translate([pinHole_x, pinHole_y, ]) pinHole();
        translate([speaker_x, speaker_y, ]) speakerHoles();
    }
    
    translate([(1-tolerance)/2*hull_x,(1-tolerance)/2*hull_y, t]) border();
    translate([separator_offset, (1-tolerance)/2*hull_y, t]) separator();
    
    translate([(1-tolerance)/2*hull_x,(1-tolerance)/2*hull_y, t]) latch();
    translate([separator_offset,(1-tolerance)/2*hull_y, t]) latch();
    translate([(1-tolerance)/2*hull_x+6, (1-tolerance)/2*hull_y+hull_y*tolerance, t]) rotate([0, 0, 180]) latch();
    translate([separator_offset+6, (1-tolerance)/2*hull_y+hull_y*tolerance, t]) rotate([0, 0, 180]) latch();
}

translate([0, 0, 0]) hull();
translate([0, -hull_y-10, 0]) color("red", 1.0) cover();
// translate([0, hull_y, hull_z+1]) rotate([180, 0, 0]) color("red", 1.0) cover();
