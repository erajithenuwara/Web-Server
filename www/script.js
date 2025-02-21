// Create floating bubbles
function createBubbles() {
    for (let i = 0; i < 15; i++) {
        const bubble = document.createElement('div');
        bubble.className = 'bubble';
        
        // Random size between 30px and 130px
        const size = Math.random() * 100 + 30;
        bubble.style.width = `${size}px`;
        bubble.style.height = `${size}px`;
        
        // Random position
        bubble.style.left = `${Math.random() * 100}vw`;
        bubble.style.top = `${Math.random() * 100}vh`;
        
        // Random animation delay
        bubble.style.animationDelay = `${Math.random() * 4}s`;
        
        document.body.appendChild(bubble);
    }
}

// Initialize bubbles when the page loads
document.addEventListener('DOMContentLoaded', createBubbles);