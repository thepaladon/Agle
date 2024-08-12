import numpy as np
from PIL import Image

from tqdm import tqdm
 

def load_cubemap_faces(input_folder, base_name):
    # Load images in the order of right, left, top, bottom, front, back
    faces = [
        Image.open(f"{input_folder}{base_name}_pos_x.png"),
        Image.open(f"{input_folder}{base_name}_neg_x.png"),
        Image.open(f"{input_folder}{base_name}_pos_y.png"),
        Image.open(f"{input_folder}{base_name}_neg_y.png"),
        Image.open(f"{input_folder}{base_name}_pos_z.png"),
        Image.open(f"{input_folder}{base_name}_neg_z.png"),
    ]
    return faces

def cubemap_to_equirectangular(faces, width, height):
    # Prepare the output image
    equirectangular = Image.new("RGB", (width, height))
    pixels = equirectangular.load()

    # Angles per pixel
    theta_step = 2 * np.pi / width
    phi_step = np.pi / height
    
    # Creating a tqdm progress bar
    pbar = tqdm(total=height, desc='Converting', leave=True)

    for y in range(height):
        for x in range(width):
            # Spherical angles
            theta = theta_step * x
            phi = phi_step * y - np.pi / 2
            
            # Spherical to Cartesian conversion
            x_c = np.cos(phi) * np.sin(theta)
            y_c = np.sin(phi)
            z_c = np.cos(phi) * np.cos(theta)
            
            # Cartesian to cubemap face
            abs_x_c, abs_y_c, abs_z_c = abs(x_c), abs(y_c), abs(z_c)
            is_x_major = abs_x_c >= abs_y_c and abs_x_c >= abs_z_c
            is_y_major = abs_y_c > abs_x_c and abs_y_c >= abs_z_c
            
            if is_x_major:
                face_idx = 0 if x_c > 0 else 1
                u = -z_c / abs_x_c if x_c > 0 else z_c / abs_x_c
                v = -y_c / abs_x_c
            elif is_y_major:
                face_idx = 2 if y_c > 0 else 3
                u = x_c / abs_y_c
                v = z_c / abs_y_c if y_c > 0 else -z_c / abs_y_c
            else:
                face_idx = 4 if z_c > 0 else 5
                u = x_c / abs_z_c if z_c > 0 else -x_c / abs_z_c
                v = -y_c / abs_z_c
            
            # Map u, v to the image coordinates
            face_width, face_height = faces[face_idx]. size
            u = int((u + 1) * face_width / 2)
            v = int((v + 1) * face_height / 2)
            u = min(max(u, 0), face_width - 1)
            v = min(max(v, 0), face_height - 1)
            
            # Get the pixel from face and assign to the equirectangular map
            pixels[x, y] = faces[face_idx].getpixel((u, v))
        
        pbar.update(1)  # Update progress after each row is processed

    pbar.close()  # Close the progress bar after the loop finishes
    return equirectangular


# Parameters
input_folder = "input/"
output_folder = "output/"
base_name = "sky"
output_width = 1024 * 2 # 8K width
output_height = 1024    # 8K height

# Load cubemap faces
cubemap_faces = load_cubemap_faces(input_folder, base_name)

# Convert to equirectangular
equirectangular_image = cubemap_to_equirectangular(cubemap_faces, output_width, output_height)

# Save the resulting equirectangular image
equirectangular_image.save(f"output/equ_{base_name}.png")

print(f"Finished rendering {base_name}.png")