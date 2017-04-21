include <../libraries-OpenSCAD/wedge.scad>

mm = 1;
$fs = 0.2*mm;
tolerance = 0.5*mm;

jst_clearance = [15,5]*mm;
board_dim = [51, 23, 0]*mm + [2,2,0]*tolerance + [0,1,0]*jst_clearance[1];
depth = 13*mm;
walls = 1.5*mm;

band_width = 22*mm + tolerance*2;
band_thickness = 3.5*mm + tolerance*2;
pin_diameter = 1*mm + tolerance; //???

display_pos = [5,6,0]*mm + [1,1,0]*tolerance;
display_dim = [15,8,0]*mm + [2,2,0]*tolerance;

button_pos = [30,-0.5,0]*mm + [1,1,0]*tolerance;
button_dim = [6,3.5,0]*mm + [2,2,0]*tolerance;

band_dim = [31,0,3]*mm + [2,0,2]*tolerance;

usb_dim = [0,8,2.25]*mm + [2,0,2]*tolerance;
usb_pos = [board_dim[0],board_dim[1]+walls-usb_dim[1]-8.5,4]*mm;

mount_d = 2.5*mm - tolerance;
mount_inset = 1.5*mm;

fingernail = 12*mm;

module shape() {
    polygon([[0,jst_clearance[1]],[jst_clearance[1],0],[board_dim[0],0],[board_dim[0],board_dim[1]],[0,board_dim[1]]]);
}

//back
translate([0,40*mm,0]) { 
    translate([0,1,0]*(board_dim[1] + jst_clearance[1])) scale([1,-1,1])
    difference() {
        linear_extrude(walls) offset(r=-tolerance) shape();
        translate(
             [1,0,0]*(board_dim[0]/2-fingernail/2)
            +[0,1,0]*(walls-1)/2
            +[0,0,1]*-1/2
        ) {
            cube([fingernail,walls+1,walls+1]);
        }
        /*translate(
             [1,0,0]*(board_dim[0]/2-fingernail/2)
            +[0,1,0]*(board_dim[1]-walls*2+0.001)
            +[0,0,1]*-1/2
        ) {
            cube([fingernail,walls+1,walls+1]);
        }*/
    }
    //pcb posts
    translate([0,1,0]*jst_clearance[1]) {
        for(x = [mount_inset+tolerance,board_dim[0]-walls*2-mount_inset+tolerance], 
            y = [mount_inset+tolerance,board_dim[1]-walls*2-mount_inset+tolerance-jst_clearance[1]]) {
            translate([1,1,0]*(mount_d/2) + [1,0,0]*x + [0,1,0]*y) {
                cylinder(d=mount_d+walls,h=depth-usb_pos[2]-tolerance);
                cylinder(d=mount_d,h=depth-walls-tolerance);
            }
        }
    }
}


//main case
union() {
    //body
    difference() {
        translate([1,1,0]*walls) linear_extrude(height=depth+walls) offset(r=walls) shape();
        translate([1,1,1]*walls) {
            difference() {
                linear_extrude(height=depth+1) shape();
                cube([board_dim[0]-jst_clearance[0], jst_clearance[1],depth-walls]);
            }
        }
        rotate([180,0,0]) translate([0,-1,0]*(board_dim[1]+walls*2) + [0,0,-1]*(walls+0.5)) {
            translate(display_pos + [1,1,0]*walls) {
                    cube(display_dim + [0,0,1]*(walls+1));
            }
            translate(button_pos + [1,1,0]*walls) {
                    cube(button_dim + [0,0,1]*(walls+1));
            }
        }
        translate(usb_pos + [1,0,0]) {
            cube(usb_dim + [1,0,0]*(walls+2));
        }
    }
    
    //lugs
    for(sy = [1,-1]) {
        translate([0,1,0]*(board_dim[1]+walls*2) * ((sy-1)/-2)) {
            scale([1,sy,1]) {
                for(x = [0,1]) {
                    translate([1,0,0]*x*(band_width + band_thickness) + 
                              [1,0,0]*((board_dim[0]+walls*2-band_width)/2 - band_thickness) +
                              [0,1,0]*-band_thickness + 
                              [0,0,1]*(depth+walls)
                              ) 
                    {
                        rotate([180,0,90]) {
                            difference() {
                                union() {
                                    cube([1,1,0]*band_thickness + [0,0,1]*(depth+walls));
                                }
                                translate([0.5,1,0.5]*band_thickness) rotate([1,0,0]*90)
                                cylinder(d=pin_diameter,h=band_thickness);
                            }
                        }
                    }
                }
            }
        }
    }
}
