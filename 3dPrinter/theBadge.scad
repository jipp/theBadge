hull_x = 90;
hull_y = 45;
hull_z = 11;

gauge = 1.2;
gauge_z = 2;
tolerance = 0.99;

$fn = 36;
// $fn = 76;

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
    thickness = 0.45;
    space = 0.6;

    difference()
    {
        union()
        {
            for (y =[0, 9, 18, 27])
            {
                translate([0, y, 0]) cube([button_x, button_y, gauge_z]);
            }
        }
        
        for (y =[0, 9, 18, 27])
        {
            translate([0, y+space, 0]) cube([button_x-space, button_y-2*space, thickness]);
        }
    }  
}

module switch()
{
    switch_x = 10;
    switch_z = 5;

    cube([switch_x, gauge, switch_z]);
    translate([0, gauge, -2.5]) cube([switch_x, 4, switch_z+2]);
}

module sd()
{
    sd_x = 16;
    sd_z = 3;

    cube([sd_x, gauge, sd_z]);
}

module usb()
{
    usb_x = 10;
    usb_z = 5;
    usb_channel = 0.75;

    cube([usb_x, gauge, usb_z]);
    translate([0, 0, usb_z]) cube([usb_x, usb_channel, 3]);
}

module holder1()
{
    translate([0, 0, 0]) cube([17.5, 1.25, 1]);
    translate([45, 0, 0]) cube([6, 1.25, 1]);
    translate([56, 0, 0]) cube([3, 1.25, 1]);
}

module holder2()
{
    translate([0, 0, 0]) cube([9, 1, 1]);
    translate([19, 0, 0]) cube([40, 1, 1]);
}

module letter()
{
    translate([0, -gauge+gauge/3, gauge_z]) rotate([90, 0, 0]) linear_extrude(gauge/3) text(text="the badge", size=2.5);
    translate([-gauge+gauge/3, 34, gauge_z]) rotate([270, 270, 90]) linear_extrude(gauge/3) text(text="reset", size=2.5);
    translate([-gauge+gauge/3, 25, gauge_z]) rotate([270, 270, 90]) linear_extrude(gauge/3) text(text="1", size=2.5);
    translate([-gauge+gauge/3, 16, gauge_z]) rotate([270, 270, 90]) linear_extrude(gauge/3) text(text="2", size=2.5);
    translate([-gauge+gauge/3, 7, gauge_z]) rotate([270, 270, 90]) linear_extrude(gauge/3) text(text="3", size=2.5);   
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
            cylinder(r=gauge, h=1);
        }
    
        translate([0, 0, gauge_z]) cube([hull_x, hull_y, hull_z-gauge_z]);

        translate([display_offset_x, display_offset_y, 0]) display();
        translate([button_offset_x,  button_offset_y, 0]) button();
        translate([switch_offset_x, -gauge, switch_offset_z]) switch();
        translate([sd_offset_x, -gauge, sd_offset_z]) sd();
        translate([usb_offset_x, hull_y, usb_offset_z]) usb();
        
        translate([(1-tolerance)/2*hull_x+3, -gauge, 7]) rotate([270, 0, 0]) cylinder(h=gauge, d=3);
        translate([(1-tolerance)/2*hull_x+3, hull_y, 7]) rotate([270, 0, 0]) cylinder(h=gauge, d=3);
        translate([65+3, -gauge, 7]) rotate([270, 0, 0]) cylinder(h=gauge, d=3);
        translate([65+3, hull_y, 7]) rotate([270, 0, 0]) cylinder(h=gauge, d=3);

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
        translate([gauge, gauge, 0]) cube([hull_x*tolerance-2*gauge, hull_y*tolerance-2*gauge, height]);
    }
}

module pinHole()
{
    pin_hole_x = 33;
    pin_hole_y = 6.5;

    cube([pin_hole_x, pin_hole_y, gauge]);
}

module speakerHoles()
{
    radius = 0.4;
    gap = 2;
    
    for (x =[-1, 0, 1])
    {
        for (y = [-1, 0, 1])
        {
            translate([-gap*x, -gap*y, 0]) cylinder(h=gauge, r=radius);
        }
    }
    
    translate([-2*gap, 0, 0]) cylinder(h=gauge, r=radius);
    translate([2*gap, 0, 0]) cylinder(h=gauge, r=radius);
}

module separator()
{
    height = 3;
    
    cube([gauge, hull_y*tolerance, height]);
}

module latch()
{
    height = hull_z - 5;
    width = 6;
    diameter = 3;
    
    cube([width, gauge, height]);
    translate([width/2, 0, hull_z - 7]) rotate([90, 0, 0]) difference()
    {
        scale ([1, 1, 0.5]) sphere(d=diameter);
        translate([0, 0, -diameter]) cube(2*diameter, center=true); 
    }
}

module cover()
{
    pinHole_x = 28;
    pinHole_y = 4;
    
    speaker_x = 45;
    speaker_y = 30;

    separator_offset = 65;
    
    difference()
    {
        minkowski()
        {
            cube([hull_x, hull_y, gauge-1]);
            cylinder(r=gauge, h=1);
        }
    
        translate([pinHole_x, pinHole_y, 0]) pinHole();
        translate([speaker_x, speaker_y, 0]) speakerHoles();
    }
    
    translate([(1-tolerance)/2*hull_x, (1-tolerance)/2*hull_y, gauge]) border();

    translate([separator_offset, (1-tolerance)/2*hull_y, gauge]) separator();
    
    translate([(1-tolerance)/2*hull_x,(1-tolerance)/2*hull_y, gauge]) latch();
    translate([separator_offset,(1-tolerance)/2*hull_y, gauge]) latch();
    translate([(1-tolerance)/2*hull_x+6, (1-tolerance)/2*hull_y+hull_y*tolerance, gauge]) rotate([0, 0, 180]) latch();
    translate([separator_offset+6, (1-tolerance)/2*hull_y+hull_y*tolerance, gauge]) rotate([0, 0, 180]) latch();
}

translate([0, 0, 0]) hull();
translate([0, -hull_y-10, 0]) color("red", 1.0) cover();
// translate([0, hull_y, hull_z+gauge]) rotate([180, 0, 0]) color("red", 1.0) cover();
