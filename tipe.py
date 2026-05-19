import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# 1. Load the full data first
df = pd.read_csv("output/simulation_data.csv")

# To prevent the animation from taking forever if you have thousands of steps,
# we can skip rows (e.g., plot every 2nd or 5th step). 
# If it runs too slow, uncomment the line below:
# df = df.iloc[::2].reset_index(drop=True)

fig, ax = plt.subplots(figsize=(10, 6))
colors = [ '#ff6b62','#2b5c65', '#4a4a4a', '#000000']  # Teal (S), Red/Coral (I), Dark Grey (R), Black (D)  

# Store maximum boundaries so the graph axes stay fixed
max_steps = df['Step'].max()
max_pop = (df['Susceptible'].max() + 10)

# This function safely updates the plot frame by frame
def update(frame):
    # 1. Clear the entire axes cleanly (fixes the AttributeError)
    ax.clear()
    
    # 2. Slice the dataframe up to the current frame index
    current_df = df.iloc[:frame]
    
    if not current_df.empty:
        # 3. Re-draw the stacked area chart up to this moment
        ax.stackplot(
            current_df['Step'], 
            current_df['Infected'], 
            current_df['Susceptible'], 
            current_df['Recovered'], 
            current_df['Dead'],
            colors=colors,
            labels=['Susceptible', 'Infected', 'Recovered', 'Dead']
        )
    
    # 4. Because ax.clear() wipes the plot settings, we re-apply boundaries here
    ax.set_xlim(0, max_steps)
    ax.set_ylim(0, max_pop)
    ax.set_xlabel('Time Steps')
    ax.set_ylabel('Agent Population')
    ax.set_title('SIR Model Simulation Output (Real-time Transmission)')
    
    # Add a legend in the upper right corner so it doesn't move around
    ax.legend(loc='upper right')

# 2. Create the animation
# interval=10 is the speed delay between frames in milliseconds (lower = faster).
ani = animation.FuncAnimation(fig, update, frames=len(df), interval=0.1, repeat=False)

plt.margins(0,0)
plt.show()