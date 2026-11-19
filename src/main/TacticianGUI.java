
package ui;

import javax.swing.*;
import java.awt.*;

public class TacticianGUI {

    public static void main(String[] args) {

        SwingUtilities.invokeLater(() -> {

            JFrame frame = new JFrame("Tactician Compiler");

            frame.setSize(1100, 750);
            frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
            frame.setLocationRelativeTo(null);
            frame.setLayout(new BorderLayout(10, 10));

            // Top panel
            JPanel topPanel = new JPanel(new BorderLayout());

            JLabel title = new JLabel("TACTICIAN COMPILER");
            title.setFont(new Font("Arial", Font.BOLD, 22));
            title.setBorder(BorderFactory.createEmptyBorder(10, 15, 10, 10));

            JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT));

            JButton openButton = new JButton("Open");
            JButton saveButton = new JButton("Save");
            JButton compileButton = new JButton("Compile");
            JButton clearButton = new JButton("Clear");

            buttonPanel.add(openButton);
            buttonPanel.add(saveButton);
            buttonPanel.add(compileButton);
            buttonPanel.add(clearButton);

            topPanel.add(title, BorderLayout.WEST);
            topPanel.add(buttonPanel, BorderLayout.EAST);

            // Source editor
            JLabel editorLabel = new JLabel("Source Code Editor");

            JTextArea sourceArea = new JTextArea();
            sourceArea.setFont(new Font("Consolas", Font.PLAIN, 16));
            sourceArea.setTabSize(4);

            JScrollPane editorScroll = new JScrollPane(sourceArea);

            JPanel editorPanel = new JPanel(new BorderLayout(5, 5));
            editorPanel.add(editorLabel, BorderLayout.NORTH);
            editorPanel.add(editorScroll, BorderLayout.CENTER);

            // Compiler output
            JLabel outputLabel = new JLabel("Compiler Output (TAC)");

            JTextArea outputArea = new JTextArea();
            outputArea.setFont(new Font("Consolas", Font.PLAIN, 14));
            outputArea.setEditable(false);

            JScrollPane outputScroll = new JScrollPane(outputArea);

            JPanel outputPanel = new JPanel(new BorderLayout(5, 5));
            outputPanel.add(outputLabel, BorderLayout.NORTH);
            outputPanel.add(outputScroll, BorderLayout.CENTER);

            // Split editor and output
            JSplitPane splitPane = new JSplitPane(
                    JSplitPane.VERTICAL_SPLIT,
                    editorPanel,
                    outputPanel
            );

            splitPane.setResizeWeight(0.65);
            splitPane.setDividerLocation(430);

            // Status bar
            JLabel statusLabel = new JLabel("Ready");
            statusLabel.setBorder(
                    BorderFactory.createEmptyBorder(8, 12, 8, 12)
            );

            frame.add(topPanel, BorderLayout.NORTH);
            frame.add(splitPane, BorderLayout.CENTER);
            frame.add(statusLabel, BorderLayout.SOUTH);

            frame.setVisible(true);

        });
    }
}
