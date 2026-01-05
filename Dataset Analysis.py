
# Visual Wake Words Dataset Analysis for TinyML Desk Activity Monitor
# Python-based data analysis using Pandas, NumPy, and Matplotlib

import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from PIL import Image
import random

from google.colab import drive
drive.mount('/content/drive')
DATASET_PATH = "/content/drive/MyDrive/visual_wake_words"

def analyse_dataset(dataset_path):
    """
    Analyse the Visual Wake Words dataset structure and statistics
    """
    print("=" * 60)
    print("VISUAL WAKE WORDS DATASET ANALYSIS")
    print("TinyML Desk Activity Monitor Project")
    print("=" * 60)

    # Define class folders
    classes = ['non_person', 'person']

    # Count images in each class
    class_counts = {}
    image_files = {}

    for class_name in classes:
        class_path = os.path.join(dataset_path, class_name)
        if os.path.exists(class_path):
            files = [f for f in os.listdir(class_path)
                    if f.lower().endswith(('.png', '.jpg', '.jpeg'))]
            class_counts[class_name] = len(files)
            image_files[class_name] = files
        else:
            print(f"Warning: {class_path} not found")
            class_counts[class_name] = 0
            image_files[class_name] = []

    # Create DataFrame for analysis
    df = pd.DataFrame({
        'Class': list(class_counts.keys()),
        'Image Count': list(class_counts.values())
    })

    # Calculate statistics
    total_images = df['Image Count'].sum()
    df['Percentage'] = (df['Image Count'] / total_images * 100).round(2)

    print("\n1. DATASET OVERVIEW")
    print("-" * 40)
    print(f"Total Images: {total_images}")
    print(f"Number of Classes: {len(classes)}")
    print(f"Image Format: RGB (96 x 96 pixels)")

    print("\n2. CLASS DISTRIBUTION")
    print("-" * 40)
    print(df.to_string(index=False))

    # Check class balance
    print("\n3. CLASS BALANCE ANALYSIS")
    print("-" * 40)
    if len(class_counts) >= 2:
        ratio = max(class_counts.values()) / max(min(class_counts.values()), 1)
        print(f"Class Ratio: {ratio:.2f}:1")
        if ratio < 1.5:
            print("Status: Dataset is well balanced")
        elif ratio < 2.0:
            print("Status: Dataset is slightly imbalanced")
        else:
            print("Status: Dataset is imbalanced, consider data augmentation")

    return df, image_files

def analyse_image_properties(dataset_path, sample_size=50):
    """
    Analyse image properties from a sample of images
    """
    print("\n4. IMAGE PROPERTIES ANALYSIS")
    print("-" * 40)

    classes = ['non_person', 'person']
    all_widths = []
    all_heights = []
    all_channels = []

    for class_name in classes:
        class_path = os.path.join(dataset_path, class_name)
        if os.path.exists(class_path):
            files = [f for f in os.listdir(class_path)
                    if f.lower().endswith(('.png', '.jpg', '.jpeg'))]

            # Sample random images
            sample_files = random.sample(files, min(sample_size, len(files)))

            for filename in sample_files:
                img_path = os.path.join(class_path, filename)
                try:
                    img = Image.open(img_path)
                    all_widths.append(img.width)
                    all_heights.append(img.height)
                    all_channels.append(len(img.getbands()))
                except Exception as e:
                    print(f"Error reading {filename}: {e}")

    if all_widths:
        print(f"Sample Size: {len(all_widths)} images")
        print(f"Width  - Min: {min(all_widths)}, Max: {max(all_widths)}, Mean: {np.mean(all_widths):.1f}")
        print(f"Height - Min: {min(all_heights)}, Max: {max(all_heights)}, Mean: {np.mean(all_heights):.1f}")
        print(f"Channels: {set(all_channels)} (3 = RGB)")

    return all_widths, all_heights

def plot_class_distribution(df, save_path=None):
    """
    Create a bar chart showing class distribution
    """
    plt.figure(figsize=(8, 6))

    colours = ['#3498db', '#e74c3c']
    bars = plt.bar(df['Class'], df['Image Count'], color=colours, edgecolor='black')

    # Add value labels on bars
    for bar, count, pct in zip(bars, df['Image Count'], df['Percentage']):
        plt.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 50,
                f'{count}\n({pct}%)', ha='center', va='bottom', fontsize=11)

    plt.xlabel('Class', fontsize=12)
    plt.ylabel('Number of Images', fontsize=12)
    plt.title('Visual Wake Words Dataset - Class Distribution', fontsize=14, fontweight='bold')
    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"\nChart saved to: {save_path}")

    plt.show()

def display_sample_images(dataset_path, image_files, samples_per_class=4, save_path=None):
    """
    Display sample images from each class
    """
    classes = ['non_person', 'person']

    fig, axes = plt.subplots(2, samples_per_class, figsize=(12, 6))
    fig.suptitle('Sample Images from Visual Wake Words Dataset', fontsize=14, fontweight='bold')

    for row, class_name in enumerate(classes):
        class_path = os.path.join(dataset_path, class_name)

        if class_name in image_files and len(image_files[class_name]) > 0:
            sample_files = random.sample(image_files[class_name],
                                        min(samples_per_class, len(image_files[class_name])))

            for col, filename in enumerate(sample_files):
                img_path = os.path.join(class_path, filename)
                try:
                    img = Image.open(img_path)
                    axes[row, col].imshow(img)
                    axes[row, col].axis('off')
                    if col == 0:
                        axes[row, col].set_ylabel(class_name, fontsize=12)
                except Exception as e:
                    axes[row, col].text(0.5, 0.5, 'Error', ha='center')
                    axes[row, col].axis('off')

        # Add row labels
        axes[row, 0].set_title(f'Class: {class_name}', loc='left', fontsize=11, fontweight='bold')

    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Sample images saved to: {save_path}")

    plt.show()

def generate_model_comparison_chart(save_path=None):
    """
    Create a comparison chart of the three trained models
    """
    # Model data from Edge Impulse training
    models = ['MobileNetV1\n0.25', 'MobileNetV1\n0.2', 'MobileNetV1\n0.1']
    accuracy = [81.8, 72.5, 64.0]
    ram_usage = [124.8, 97.4, 58.5]
    inference_time = [691, 748, 324]

    fig, axes = plt.subplots(1, 3, figsize=(14, 5))
    fig.suptitle('Model Comparison - TinyML Desk Activity Monitor', fontsize=14, fontweight='bold')

    colours = ['#2ecc71', '#f39c12', '#e74c3c']

    # Accuracy chart
    bars1 = axes[0].bar(models, accuracy, color=colours, edgecolor='black')
    axes[0].set_ylabel('Accuracy (%)', fontsize=11)
    axes[0].set_title('Validation Accuracy', fontsize=12)
    axes[0].set_ylim(0, 100)
    for bar, val in zip(bars1, accuracy):
        axes[0].text(bar.get_x() + bar.get_width()/2, bar.get_height() + 2,
                    f'{val}%', ha='center', fontsize=10)

    # RAM usage chart
    bars2 = axes[1].bar(models, ram_usage, color=colours, edgecolor='black')
    axes[1].set_ylabel('RAM Usage (KB)', fontsize=11)
    axes[1].set_title('Peak RAM Usage', fontsize=12)
    for bar, val in zip(bars2, ram_usage):
        axes[1].text(bar.get_x() + bar.get_width()/2, bar.get_height() + 2,
                    f'{val}', ha='center', fontsize=10)

    # Inference time chart
    bars3 = axes[2].bar(models, inference_time, color=colours, edgecolor='black')
    axes[2].set_ylabel('Inference Time (ms)', fontsize=11)
    axes[2].set_title('Inference Latency', fontsize=12)
    for bar, val in zip(bars3, inference_time):
        axes[2].text(bar.get_x() + bar.get_width()/2, bar.get_height() + 10,
                    f'{val}', ha='center', fontsize=10)

    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Model comparison chart saved to: {save_path}")

    plt.show()

def create_training_summary_table():
    """
    Create a summary table of all model training results
    """
    print("\n5. MODEL TRAINING SUMMARY")
    print("-" * 40)

    data = {
        'Model': ['MobileNetV1 0.25', 'MobileNetV1 0.2', 'MobileNetV1 0.1'],
        'Val Accuracy': ['81.8%', '72.5%', '64.0%'],
        'Test Accuracy': ['68.04%', 'N/A', '57.18%'],
        'RAM (KB)': [124.8, 97.4, 58.5],
        'Flash (KB)': [283.6, 204.9, 95.5],
        'Latency (ms)': [691, 748, 324],
        'Deployment': ['Failed', 'Failed', 'Failed']
    }

    df = pd.DataFrame(data)
    print(df.to_string(index=False))

    return df

# Main execution
if __name__ == "__main__":

    # Check if dataset exists, if not use demo mode
    if os.path.exists(DATASET_PATH):
        print("Dataset found. Running full analysis...\n")

        # Run dataset analysis
        df, image_files = analyse_dataset(DATASET_PATH)

        # Analyse image properties
        analyse_image_properties(DATASET_PATH)

        # Plot class distribution
        plot_class_distribution(df, save_path='class_distribution.png')

        # Display sample images
        display_sample_images(DATASET_PATH, image_files, save_path='sample_images.png')

    else:
        print(f"Dataset not found at: {DATASET_PATH}")
        print("Running in DEMO MODE with Edge Impulse training results...\n")

        # Create demo data based on Edge Impulse project
        demo_data = {
            'Class': ['non_person', 'person'],
            'Image Count': [5000, 5000],  # Approximate VWW distribution
            'Percentage': [50.0, 50.0]
        }
        df = pd.DataFrame(demo_data)

        print("=" * 60)
        print("VISUAL WAKE WORDS DATASET ANALYSIS")
        print("TinyML Desk Activity Monitor Project")
        print("=" * 60)

        print("\n1. DATASET OVERVIEW")
        print("-" * 40)
        print(f"Total Images: ~10,000 (subset used in Edge Impulse)")
        print(f"Number of Classes: 2")
        print(f"Image Format: RGB (96 x 96 pixels)")

        print("\n2. CLASS DISTRIBUTION")
        print("-" * 40)
        print(df.to_string(index=False))

        print("\n3. CLASS BALANCE ANALYSIS")
        print("-" * 40)
        print("Class Ratio: 1.0:1")
        print("Status: Dataset is well balanced")

        print("\n4. IMAGE PROPERTIES")
        print("-" * 40)
        print("Width: 96 pixels")
        print("Height: 96 pixels")
        print("Channels: 3 (RGB)")
        print("Input Shape: (96, 96, 3)")

    # Always show model training summary
    create_training_summary_table()

    # Always generate model comparison chart
    generate_model_comparison_chart(save_path='model_comparison.png')

    print("\n" + "=" * 60)
    print("ANALYSIS COMPLETE")
    print("=" * 60)
    print("\nGenerated files:")
    print("- class_distribution.png (if dataset available)")
    print("- sample_images.png (if dataset available)")
    print("- model_comparison.png")
