
package ui;

import javax.swing.*;
import javax.swing.filechooser.FileNameExtensionFilter;
import java.awt.*;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;

public class TacticianGUI {

    // ================= GUI COMPONENTS =================

    private static JTextArea sourceArea;
    private static JTextArea outputArea;
    private static JLabel statusLabel;

    private static JButton compileButton;

    // Currently opened source file
    private static File currentFile;

    // Compiler executable location
    private static final Path COMPILER_ROOT =
            Paths.get("..", "Tactician-compiler").toAbsolutePath().normalize();

    private static final Path COMPILER_EXE =
            COMPILER_ROOT.resolve("build").resolve("tactician-tac.exe");

    public static void main(String[] args) {

        SwingUtilities.invokeLater(() -> {

            JFrame frame = new JFrame("Tactician Compiler");

            frame.setSize(1100, 750);
            frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
            frame.setLocationRelativeTo(null);
            frame.setLayout(new BorderLayout(10, 10));

            // ================= TOP PANEL =================

            JPanel topPanel = new JPanel(new BorderLayout());

            JLabel title = new JLabel("TACTICIAN COMPILER");
            title.setFont(new Font("Arial", Font.BOLD, 22));
            title.setBorder(
                    BorderFactory.createEmptyBorder(10, 15, 10, 10)
            );

            JPanel buttonPanel = new JPanel(
                    new FlowLayout(FlowLayout.RIGHT)
            );

            JButton openButton = new JButton("Open");
            JButton saveButton = new JButton("Save");
            JButton saveAsButton = new JButton("Save As");

            compileButton = new JButton("Compile");

            JButton clearButton = new JButton("Clear");

            buttonPanel.add(openButton);
            buttonPanel.add(saveButton);
            buttonPanel.add(saveAsButton);
            buttonPanel.add(compileButton);
            buttonPanel.add(clearButton);

            topPanel.add(title, BorderLayout.WEST);
            topPanel.add(buttonPanel, BorderLayout.EAST);

            // ================= SOURCE EDITOR =================

            JLabel editorLabel = new JLabel("Source Code Editor");

            sourceArea = new JTextArea();
            sourceArea.setFont(new Font("Consolas", Font.PLAIN, 16));
            sourceArea.setTabSize(4);

            JScrollPane editorScroll = new JScrollPane(sourceArea);

            JPanel editorPanel = new JPanel(new BorderLayout(5, 5));
            editorPanel.add(editorLabel, BorderLayout.NORTH);
            editorPanel.add(editorScroll, BorderLayout.CENTER);

            // ================= OUTPUT PANEL =================

            JLabel outputLabel = new JLabel("Compiler Output (TAC)");

            outputArea = new JTextArea();
            outputArea.setFont(new Font("Consolas", Font.PLAIN, 14));
            outputArea.setEditable(false);

            JScrollPane outputScroll = new JScrollPane(outputArea);

            JPanel outputPanel = new JPanel(new BorderLayout(5, 5));
            outputPanel.add(outputLabel, BorderLayout.NORTH);
            outputPanel.add(outputScroll, BorderLayout.CENTER);

            // ================= SPLIT PANEL =================

            JSplitPane splitPane = new JSplitPane(
                    JSplitPane.VERTICAL_SPLIT,
                    editorPanel,
                    outputPanel
            );

            splitPane.setResizeWeight(0.65);
            splitPane.setDividerLocation(430);

            // ================= STATUS BAR =================

            statusLabel = new JLabel("Ready");

            statusLabel.setBorder(
                    BorderFactory.createEmptyBorder(8, 12, 8, 12)
            );

            JPanel statusPanel = new JPanel(new BorderLayout());

            statusPanel.setBorder(
                    BorderFactory.createMatteBorder(
                            1, 0, 0, 0, Color.LIGHT_GRAY
                    )
            );

            statusPanel.add(statusLabel, BorderLayout.WEST);

            // ================= BUTTON ACTIONS =================

            openButton.addActionListener(e -> openFile(frame));

            saveButton.addActionListener(e -> saveFile(frame, false));

            saveAsButton.addActionListener(e -> saveFile(frame, true));

            clearButton.addActionListener(e -> clearEditor());

            compileButton.addActionListener(e -> compileProgram(frame));

            // ================= ADD COMPONENTS =================

            frame.add(topPanel, BorderLayout.NORTH);
            frame.add(splitPane, BorderLayout.CENTER);
            frame.add(statusPanel, BorderLayout.SOUTH);

            frame.setVisible(true);
        });
    }

    // ================= OPEN FILE =================

    private static void openFile(JFrame frame) {

        JFileChooser chooser = new JFileChooser();

        FileNameExtensionFilter filter =
                new FileNameExtensionFilter(
                        "Tactician Files (*.tac)", "tac"
                );

        chooser.setFileFilter(filter);

        int result = chooser.showOpenDialog(frame);

        if (result == JFileChooser.APPROVE_OPTION) {

            File selectedFile = chooser.getSelectedFile();

            try {

                String content = Files.readString(
                        selectedFile.toPath(),
                        StandardCharsets.UTF_8
                );

                sourceArea.setText(content);
                currentFile = selectedFile;

                outputArea.setText("");

                statusLabel.setText(
                        "Opened: " + currentFile.getName()
                );

            } catch (IOException ex) {

                JOptionPane.showMessageDialog(
                        frame,
                        "Could not open file: " + ex.getMessage(),
                        "File Error",
                        JOptionPane.ERROR_MESSAGE
                );
            }
        }
    }

    // ================= SAVE AND SAVE AS =================

    private static boolean saveFile(JFrame frame, boolean saveAs) {

        if (currentFile == null || saveAs) {

            JFileChooser chooser = new JFileChooser();

            FileNameExtensionFilter filter =
                    new FileNameExtensionFilter(
                            "Tactician Files (*.tac)", "tac"
                    );

            chooser.setFileFilter(filter);

            if (currentFile != null) {
                chooser.setSelectedFile(currentFile);
            }

            int result = chooser.showSaveDialog(frame);

            if (result != JFileChooser.APPROVE_OPTION) {
                return false;
            }

            File selectedFile = chooser.getSelectedFile();

            if (!selectedFile.getName().toLowerCase().endsWith(".tac")) {

                selectedFile = new File(
                        selectedFile.getAbsolutePath() + ".tac"
                );
            }

            if (selectedFile.exists()) {

                int choice = JOptionPane.showConfirmDialog(
                        frame,
                        "This file already exists. Overwrite it?",
                        "Confirm Overwrite",
                        JOptionPane.YES_NO_OPTION,
                        JOptionPane.WARNING_MESSAGE
                );

                if (choice != JOptionPane.YES_OPTION) {
                    return false;
                }
            }

            currentFile = selectedFile;
        }

        try {

            Files.writeString(
                    currentFile.toPath(),
                    sourceArea.getText(),
                    StandardCharsets.UTF_8
            );

            statusLabel.setText(
                    "Saved: " + currentFile.getName()
            );

            return true;

        } catch (IOException ex) {

            JOptionPane.showMessageDialog(
                    frame,
                    "Could not save file: " + ex.getMessage(),
                    "File Error",
                    JOptionPane.ERROR_MESSAGE
            );

            return false;
        }
    }

    // ================= CLEAR EDITOR =================

    private static void clearEditor() {

        int choice = JOptionPane.showConfirmDialog(
                null,
                "Are you sure you want to clear the editor and output?",
                "Clear Editor",
                JOptionPane.YES_NO_OPTION,
                JOptionPane.WARNING_MESSAGE
        );

        if (choice == JOptionPane.YES_OPTION) {

            sourceArea.setText("");
            outputArea.setText("");

            currentFile = null;

            statusLabel.setText("Editor cleared. Ready.");
        }
    }

    // ================= COMPILER RESULT =================

    private static class CompileResult {

        String standardOutput;
        String standardError;
        int exitCode;

        CompileResult(
                String standardOutput,
                String standardError,
                int exitCode
        ) {

            this.standardOutput = standardOutput;
            this.standardError = standardError;
            this.exitCode = exitCode;
        }
    }

    // ================= READ PROCESS OUTPUT =================

    private static String readStream(InputStream stream)
            throws IOException {

        return new String(
                stream.readAllBytes(),
                StandardCharsets.UTF_8
        );
    }

    // ================= COMPILE PROGRAM =================

    private static void compileProgram(JFrame frame) {

        // A source file must be saved before compilation
        if (currentFile == null) {

            boolean saved = saveFile(frame, true);

            if (!saved) {
                return;
            }
        }

        // Save the latest editor contents before compiling
        if (!saveFile(frame, false)) {
            return;
        }

        // Check whether the compiler executable exists
        if (!Files.isRegularFile(COMPILER_EXE)) {

            outputArea.setText(
                    "ERROR: Compiler executable was not found.\n\n"
                    + "Expected location:\n"
                    + COMPILER_EXE
            );

            statusLabel.setText("Compiler executable not found.");
            return;
        }

        outputArea.setText("Compiling program...\n");

        statusLabel.setText("Compilation in progress...");

        compileButton.setEnabled(false);

        // Run compiler in the background
        SwingWorker<CompileResult, Void> worker =
                new SwingWorker<CompileResult, Void>() {

            @Override
            protected CompileResult doInBackground() throws Exception {

                // Get the source file's absolute path
Path sourcePath = currentFile.toPath()
        .toAbsolutePath()
        .normalize();

// Get the compiler's root directory
Path compilerRoot = COMPILER_ROOT
        .toAbsolutePath()
        .normalize();

// Check whether the source file is inside the compiler project
if (!sourcePath.startsWith(compilerRoot)) {
    throw new IOException(
            "Please open a source file from the Tactician-compiler project."
    );
}

// Convert the absolute path into a relative path
Path relativeSourcePath = compilerRoot.relativize(sourcePath);

// Execute the compiler using the relative source path
ProcessBuilder builder = new ProcessBuilder(
        COMPILER_EXE.toString(),
        relativeSourcePath.toString()
);

// Set the compiler's working directory
builder.directory(compilerRoot.toFile());


                Process process = builder.start();

                // Read both streams concurrently to avoid blocking
                CompletableFuture<String> stdoutFuture =
                        CompletableFuture.supplyAsync(() -> {

                    try {
                        return readStream(process.getInputStream());
                    } catch (IOException ex) {
                        throw new RuntimeException(ex);
                    }
                });

                CompletableFuture<String> stderrFuture =
                        CompletableFuture.supplyAsync(() -> {

                    try {
                        return readStream(process.getErrorStream());
                    } catch (IOException ex) {
                        throw new RuntimeException(ex);
                    }
                });

                int exitCode = process.waitFor();

                String stdout = stdoutFuture.get();
                String stderr = stderrFuture.get();

                return new CompileResult(
                        stdout,
                        stderr,
                        exitCode
                );
            }

            @Override
            protected void done() {

                compileButton.setEnabled(true);

                try {

                    CompileResult result = get();

                    StringBuilder output = new StringBuilder();

                    if (!result.standardOutput.isEmpty()) {

                        output.append(result.standardOutput);
                    }

                    if (!result.standardError.isEmpty()) {

                        if (output.length() > 0) {
                            output.append("\n");
                        }

                        output.append("Compiler Errors:\n");
                        output.append(result.standardError);
                    }

                    if (output.length() == 0) {

                        output.append(
                                "Compiler finished without producing output."
                        );
                    }

                    outputArea.setText(output.toString());

                    outputArea.setCaretPosition(0);

                    if (result.exitCode == 0) {

                        statusLabel.setText(
                                "Compilation successful!"
                        );

                        JOptionPane.showMessageDialog(
                                frame,
                                "Compilation completed successfully!",
                                "Compilation Successful",
                                JOptionPane.INFORMATION_MESSAGE
                        );

                    } else {

                        statusLabel.setText(
                                "Compilation failed. Exit code: "
                                        + result.exitCode
                        );

                        JOptionPane.showMessageDialog(
                                frame,
                                "Compilation failed. Check the output panel.",
                                "Compilation Error",
                                JOptionPane.ERROR_MESSAGE
                        );
                    }

                } catch (InterruptedException ex) {

                    Thread.currentThread().interrupt();

                    outputArea.setText(
                            "Compilation was interrupted."
                    );

                    statusLabel.setText("Compilation interrupted.");

                } catch (ExecutionException ex) {

                    Throwable cause = ex.getCause();

                    outputArea.setText(
                            "Could not execute compiler:\n"
                                    + cause.getMessage()
                    );

                    statusLabel.setText("Compiler execution error.");
                }
            }
        };

        worker.execute();
    }
}
