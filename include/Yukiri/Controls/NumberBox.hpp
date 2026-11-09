#pragma once

#include <Yukiri/Controls/TextBox.hpp>

namespace Yukiri
{
class Button;
class NumberBox : public Widget
{
public:
	enum class Format { General, Fixed, Scientific };
	explicit NumberBox(double value = 0);
	[[nodiscard]] Result<void> SetRange(double minimum, double maximum);
	[[nodiscard]] Result<void> SetStep(double step);
	[[nodiscard]] Result<void> SetFormat(Format format, int precision = 6);
	void SetValue(double value);
	void SetReadOnly(bool readOnly);
	[[nodiscard]] bool IsReadOnly() const noexcept;
	[[nodiscard]] double GetValue() const noexcept;
	[[nodiscard]] double GetMinimum() const noexcept;
	[[nodiscard]] double GetMaximum() const noexcept;
	[[nodiscard]] double GetStep() const noexcept;
	[[nodiscard]] bool IsValid() const noexcept;
	[[nodiscard]] bool Commit();
	void Revert();
	void StepBy(int steps);
	[[nodiscard]] TextBox& GetEditor() noexcept;
	void OnPreviewKeyDown(KeyEvent& event) override;
	void OnFocusChanged(bool focused) override;
	Signal<double> m_ValueChanged;
	Signal<bool> m_ValidationChanged;
protected:
	Size OnMeasure(Size available) override;
	void OnArrange(Rect content) override;
private:
	class Editor;
	[[nodiscard]] std::optional<double> Parse() const;
	void UpdateValidity();
	void FormatValue();
	TextBox* m_Editor;
	Button* m_Up;
	Button* m_Down;
	double m_Value = 0;
	double m_Minimum = -1.7976931348623157e308;
	double m_Maximum = 1.7976931348623157e308;
	double m_Step = 1;
	Format m_Format = Format::General;
	int m_Precision = 6;
	bool m_Valid = true;
};
}
