from PIL import Image, ImageDraw, ImageFont
import os

def create_sample_image(index, width=800, height=600):
    # Create new image with gradient background
    image = Image.new('RGB', (width, height))
    draw = ImageDraw.Draw(image)
    
    # Fill with gradient
    for y in range(height):
        r = int(255 * y / height)
        g = int(128 * y / height)
        b = int(64 * y / height)
        draw.line([(0, y), (width, y)], fill=(r, g, b))
    
    # Add text
    text = f"Sample Image {index}"
    font_size = 60
    try:
        font = ImageFont.truetype("Arial", font_size)
    except:
        font = ImageFont.load_default()
    
    # Draw text
    text_bbox = draw.textbbox((0, 0), text, font=font)
    text_width = text_bbox[2] - text_bbox[0]
    text_height = text_bbox[3] - text_bbox[1]
    
    x = (width - text_width) // 2
    y = (height - text_height) // 2
    draw.text((x, y), text, fill='white', font=font)
    
    return image

# Create directories if they don't exist
os.makedirs('data/images', exist_ok=True)

# Generate sample images
for i in range(1, 6):
    img = create_sample_image(i)
    img.save(f'data/images/img{i}.jpg', quality=85, optimize=True)
    print(f"Created image {i}")
