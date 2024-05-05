m = 0.8;
f = 20:0.1:120;
A = @(f,p) sqrt((p*8*pi.^2)./(m*f.^3));
figure;
for p = 0:8:48
    result = A(f,p);
    plot(f, result, 'DisplayName', ['p = ', num2str(p), ' watts']) % Include p value in legend
    hold on
end
xlabel('Frequency (Hz)')
ylabel('Amplitude (cm)')
title('Plot of Amplitude vs. Frequency for varying Power')
legend('show')